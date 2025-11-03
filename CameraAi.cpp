#include "CameraAI.hpp"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <filesystem>
#include <algorithm>
#include <torch/script.h>
#include <torch/torch.h>

CameraAI::CameraAI(QObject* parent)
    : QObject(parent), running(false),
    grid_(rows_, QVector<int>(cols_, 0)), gridComplete_(false)
{
    connect(&workerThread, &QThread::started, this, &CameraAI::processLoop);
    moveToThread(&workerThread);

    QString modelPath = QCoreApplication::applicationDirPath() + "/Model/best7.torchscript";
    qDebug() << "[AI] Chargement du modèle :" << modelPath;

    std::filesystem::path fsPath = modelPath.toStdWString();
    if (!std::filesystem::exists(fsPath)) {
        qWarning() << "[AI] ⚠️ Modèle introuvable:" << modelPath;
        return;
    }

    try {
        model = std::make_shared<torch::jit::Module>(torch::jit::load(fsPath.string(), torch::kCPU));
        model->eval();
        qDebug() << "[AI] ✅ Modèle YOLOv8 TorchScript chargé (CPU)";
    } catch (const c10::Error& e) {
        qWarning() << "[AI] ❌ Erreur de chargement:" << e.what();
    }
}

CameraAI::~CameraAI() {
    stop();
}

void CameraAI::start(int camIndex) {
    if (!cap.open(camIndex, cv::CAP_DSHOW)) {
        qWarning() << "[AI] ❌ Impossible d'ouvrir la caméra (index:" << camIndex << ")";
        return;
    }
    running = true;
    workerThread.start();
    qDebug() << "[AI] 🚀 Capture démarrée";
}

void CameraAI::stop() {
    running = false;
    if (workerThread.isRunning()) {
        workerThread.quit();
        workerThread.wait();
    }
    if (cap.isOpened()) cap.release();
    qDebug() << "[AI] 🛑 Capture arrêtée";
}

int CameraAI::getGrille(QVector<QVector<int>>& out) const {
    QMutexLocker lock(&gridMutex_);
    if (!gridComplete_) return -1;
    out = grid_; // copie
    return 0;
}

void CameraAI::processLoop() {
    try {
        cv::Mat frame;
        while (running) {
            cap >> frame;
            if (frame.empty()) continue;

            auto dets = inferTorch(frame);
            updateGrid(dets); // met à jour l'état de la grille

            QImage img = matToQImage(frame);
            emit frameReady(img);

            QThread::msleep(30);
        }
    } catch (const std::exception& e) {
        qWarning() << "[AI] ❌ Exception:" << e.what();
    }
}

std::vector<Detection> CameraAI::inferTorch(const cv::Mat& frameBGR) {
    std::vector<Detection> results;
    if (!model || frameBGR.empty()) return results;

    // --- 1) LetterBox
    const int imgsz = 960;
    const cv::Scalar padColor(114,114,114);
    int h0 = frameBGR.rows, w0 = frameBGR.cols;
    float gain = std::min((float)imgsz / h0, (float)imgsz / w0);
    int newW  = (int)std::round(w0 * gain);
    int newH  = (int)std::round(h0 * gain);
    int padW  = imgsz - newW;
    int padH  = imgsz - newH;
    int dw = padW / 2, dh = padH / 2;

    cv::Mat resized, lb(imgsz, imgsz, CV_8UC3, padColor);
    cv::resize(frameBGR, resized, cv::Size(newW, newH), 0, 0, cv::INTER_LINEAR);
    resized.copyTo(lb(cv::Rect(dw, dh, newW, newH)));

    // --- 2) RGB + normalize
    cv::cvtColor(lb, lb, cv::COLOR_BGR2RGB);
    lb.convertTo(lb, CV_32F, 1.0/255.0);
    at::Tensor input = torch::from_blob(lb.data, {1, imgsz, imgsz, 3}, at::kFloat)
                           .permute({0,3,1,2}).contiguous();

    // --- 3) Inference
    torch::NoGradGuard noGrad;
    at::Tensor out;
    try {
        out = model->forward({input}).toTensor();
    } catch (const c10::Error& e) {
        qWarning() << "[AI] ❌ Erreur inférence:" << e.what();
        return results;
    }

    // --- 4) Process output
    out = out.squeeze(0).permute({1,0});            // [N, 4+nc]
    int64_t nc = out.size(1) - 4;
    at::Tensor boxes_xywh = out.slice(1, 0, 4);
    at::Tensor cls_scores = out.slice(1, 4, 4 + nc);

    // --- 5) Confiance & filtre
    at::Tensor conf, labels;
    std::tie(conf, labels) = cls_scores.max(1);
    const float confTh = 0.25f;
    at::Tensor keep = conf > confTh;
    boxes_xywh = boxes_xywh.index({keep});
    conf       = conf.index({keep});
    labels     = labels.index({keep});

    // --- 6) Convert xywh -> xyxy
    auto x = boxes_xywh.select(1,0), y = boxes_xywh.select(1,1);
    auto w = boxes_xywh.select(1,2), h = boxes_xywh.select(1,3);
    at::Tensor xyxy = at::stack({x - w*0.5, y - h*0.5, x + w*0.5, y + h*0.5}, 1);

    // --- 7) NMS par classe
    const float iouTh = 0.45f;
    std::vector<cv::Rect> finalBoxes;
    std::vector<float> finalScores;
    std::vector<int>   finalClasses;

    auto xyxy_cpu = xyxy.to(torch::kCPU);
    auto conf_cpu = conf.to(torch::kCPU);
    auto lab_cpu  = labels.to(torch::kCPU);

    for (int c = 0; c < nc; ++c) {
        std::vector<cv::Rect> boxesC;
        std::vector<float>    scoresC;

        for (int i = 0; i < xyxy_cpu.size(0); ++i) {
            if (lab_cpu[i].item<int>() != c) continue;
            float x1 = xyxy_cpu[i][0].item<float>();
            float y1 = xyxy_cpu[i][1].item<float>();
            float x2 = xyxy_cpu[i][2].item<float>();
            float y2 = xyxy_cpu[i][3].item<float>();
            boxesC.push_back(cv::Rect(cv::Point((int)x1,(int)y1), cv::Point((int)x2,(int)y2)));
            scoresC.push_back(conf_cpu[i].item<float>());
        }
        if (boxesC.empty()) continue;

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxesC, scoresC, confTh, iouTh, indices);
        for (int idx : indices) {
            finalBoxes.push_back(boxesC[idx]);
            finalScores.push_back(scoresC[idx]);
            finalClasses.push_back(c);
        }
    }

    // --- 8) Remise à l’échelle vers image d’origine
    for (size_t i = 0; i < finalBoxes.size(); ++i) {
        float x1 = (finalBoxes[i].x - dw) / gain;
        float y1 = (finalBoxes[i].y - dh) / gain;
        float x2 = (finalBoxes[i].x + finalBoxes[i].width  - dw) / gain;
        float y2 = (finalBoxes[i].y + finalBoxes[i].height - dh) / gain;

        x1 = std::clamp(x1, 0.f, (float)w0);
        y1 = std::clamp(y1, 0.f, (float)h0);
        x2 = std::clamp(x2, 0.f, (float)w0);
        y2 = std::clamp(y2, 0.f, (float)h0);

        finalBoxes[i] = cv::Rect(cv::Point((int)x1,(int)y1), cv::Point((int)x2,(int)y2));
    }

    static const std::vector<std::string> names = {"r","y","e"};

    // --- 9) Dessin + remplir 'results'
    results.reserve(finalBoxes.size());
    for (size_t i = 0; i < finalBoxes.size(); ++i) {
        int cls = finalClasses[i];
        float sc = finalScores[i];

        // push dans le vecteur retourné
        Detection d;
        d.x1   = (float)finalBoxes[i].x;
        d.y1   = (float)finalBoxes[i].y;
        d.x2   = (float)(finalBoxes[i].x + finalBoxes[i].width);
        d.y2   = (float)(finalBoxes[i].y + finalBoxes[i].height);
        d.conf = sc;
        d.cls  = cls; // 0=r,1=y,2=e
        results.push_back(d);

        // Dessin
        cv::Scalar color = (cls==0 ? cv::Scalar(0,0,255) :
                                cls==1 ? cv::Scalar(0,255,255) :
                                cv::Scalar(255,255,255));
        cv::rectangle((cv::Mat&)frameBGR, finalBoxes[i], color, 2);

        char txt[64];
        std::snprintf(txt, sizeof(txt), "%s (%.2f)", names[cls].c_str(), sc);
        int base;
        auto sz = cv::getTextSize(txt, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &base);
        cv::rectangle((cv::Mat&)frameBGR,
                      cv::Rect(finalBoxes[i].x, std::max(0, finalBoxes[i].y - sz.height - 6),
                               sz.width + 6, sz.height + 6),
                      color, cv::FILLED);
        cv::putText((cv::Mat&)frameBGR, txt,
                    {finalBoxes[i].x + 3, finalBoxes[i].y - 3},
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,0), 2);
    }

    return results;
}

void CameraAI::updateGrid(const std::vector<Detection>& dets) {
    // On attend 6x7 = 42 cellules (r, y ou e). Sinon -> grille incomplète.
    if ((int)dets.size() != rows_ * cols_) {
        QMutexLocker lock(&gridMutex_);
        gridComplete_ = false;
        return;
    }

    struct Cell { float cx, cy; int val; };
    std::vector<Cell> cells;
    cells.reserve(dets.size());

    // mapping: det.cls 0=r,1=y,2=e -> 1,2,0
    auto mapVal = [](int cls)->int {
        if (cls == 2) return 0; // empty
        if (cls == 0) return 1; // red
        return 2;               // yellow
    };

    for (const auto& d : dets) {
        float cx = 0.5f * (d.x1 + d.x2);
        float cy = 0.5f * (d.y1 + d.y2);
        cells.push_back({cx, cy, mapVal(d.cls)});
    }

    // Trier par Y (du haut vers le bas), puis découper en 6 lignes de 7
    std::sort(cells.begin(), cells.end(),
              [](const Cell& a, const Cell& b){ return a.cy < b.cy; });

    QVector<QVector<int>> newGrid(rows_, QVector<int>(cols_, 0));

    bool ok = true;
    for (int r = 0; r < rows_; ++r) {
        int start = r * cols_;
        int end   = start + cols_;
        if (end > (int)cells.size()) { ok = false; break; }

        // trier la "ligne" par X (gauche->droite)
        std::sort(cells.begin() + start, cells.begin() + end,
                  [](const Cell& a, const Cell& b){ return a.cx < b.cx; });

        for (int c = 0; c < cols_; ++c) {
            newGrid[r][c] = cells[start + c].val;
        }
    }

    QMutexLocker lock(&gridMutex_);
    grid_ = std::move(newGrid);
    gridComplete_ = ok;
    if (gridComplete_) emit gridUpdated(grid_);
}

QImage CameraAI::matToQImage(const cv::Mat& mat) {
    if (mat.empty()) return {};
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}
