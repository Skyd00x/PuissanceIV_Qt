#include "CameraAI.hpp"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <filesystem>
#include <torch/script.h>
#include <torch/torch.h>

CameraAI::CameraAI(QObject* parent)
    : QObject(parent), running(false)
{
    connect(&workerThread, &QThread::started, this, &CameraAI::processLoop);
    moveToThread(&workerThread);

    QString modelPath = QCoreApplication::applicationDirPath() + "/Model/best.torchscript";
    qDebug() << "[AI] Chemin absolu du modèle :" << modelPath;

    std::filesystem::path fsPath = modelPath.toStdWString();
    if (!std::filesystem::exists(fsPath)) {
        qWarning() << "[AI] ⚠️ Fichier modèle introuvable:" << modelPath;
        return;
    }

    try {
        model = std::make_shared<torch::jit::Module>(torch::jit::load(fsPath.string(), torch::kCPU));
        model->eval();
        qDebug() << "[AI] ✅ Modèle YOLOv8 TorchScript chargé (CPU)";
    } catch (const c10::Error& e) {
        qWarning() << "[AI] ❌ Erreur lors du chargement du modèle:" << e.what();
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
    qDebug() << "[AI] 🚀 Capture démarrée (thread LibTorch)";
}

void CameraAI::stop() {
    running = false;
    if (workerThread.isRunning()) {
        workerThread.quit();
        workerThread.wait();
    }
    if (cap.isOpened()) cap.release();
    qDebug() << "[AI] 🛑 Capture arrêtée proprement";
}

void CameraAI::processLoop() {
    try {
        cv::Mat frame;
        while (running) {
            cap >> frame;
            if (frame.empty()) continue;

            auto detections = inferTorch(frame);

            for (const auto& det : detections) {
                cv::rectangle(frame,
                              cv::Point(det.x1, det.y1),
                              cv::Point(det.x2, det.y2),
                              cv::Scalar(0, 255, 0), 2);
            }

            emit frameReady(matToQImage(frame));
            QThread::msleep(30);
        }
    } catch (const c10::Error& e) {
        qWarning() << "[AI] ❌ Exception C10:" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "[AI] ❌ Exception C++:" << e.what();
    } catch (...) {
        qWarning() << "[AI] ❌ Erreur inconnue dans processLoop";
    }
}

std::vector<Detection> CameraAI::inferTorch(const cv::Mat& frame) {
    std::vector<Detection> results;

    if (!model) {
        qWarning() << "[AI] ⚠️ Modèle non chargé — inférence ignorée";
        return results;
    }

    // Prétraitement identique YOLO
    cv::Mat img;
    cv::cvtColor(frame, img, cv::COLOR_BGR2RGB);
    cv::resize(img, img, cv::Size(640, 640));
    img.convertTo(img, CV_32F, 1.0 / 255.0);
    torch::Tensor inputTensor = torch::from_blob(img.data, {1, img.rows, img.cols, 3}, torch::kFloat32)
                                    .permute({0, 3, 1, 2})
                                    .to(torch::kCPU);

    torch::NoGradGuard noGrad;

    try {
        // === Inférence brute ===
        auto out = model->forward({inputTensor}).toTensor(); // [1, 7, 8400]
        out = out.squeeze(0).permute({1, 0});                // [8400, 7]

        // Convertir xywh -> xyxy
        auto xywh = out.slice(1, 0, 4);
        auto conf = out.select(1, 4).unsqueeze(1);
        auto cls = out.slice(1, 5); // [8400, 3]

        // conf * cls (comme Ultralytics)
        auto scores = cls * conf;

        // Trouver meilleure classe
        auto max_scores = std::get<0>(scores.max(1));
        auto max_classes = std::get<1>(scores.max(1));

        // Seuillage confiance (identique YOLO Python)
        float conf_thresh = 0.25f;
        auto mask = max_scores > conf_thresh;
        auto inds = mask.nonzero().squeeze();
        if (inds.numel() == 0) return results;

        auto boxes = xywh.index_select(0, inds);
        auto scores_sel = max_scores.index_select(0, inds);
        auto classes_sel = max_classes.index_select(0, inds);

        // xywh → xyxy
        auto x = boxes.select(1, 0);
        auto y = boxes.select(1, 1);
        auto w = boxes.select(1, 2);
        auto h = boxes.select(1, 3);
        auto x1 = x - w / 2;
        auto y1 = y - h / 2;
        auto x2 = x + w / 2;
        auto y2 = y + h / 2;
        auto xyxy = torch::stack({x1, y1, x2, y2}, 1);

        // === NMS simple (IoU 0.45)
        const float iou_thresh = 0.45f;
        std::vector<int> keep;
        auto idxs = std::get<1>(scores_sel.sort(0, true)).to(torch::kCPU);
        auto boxes_cpu = xyxy.to(torch::kCPU);
        auto scores_cpu = scores_sel.to(torch::kCPU);

        while (idxs.numel() > 0) {
            int i = idxs[0].item<int>();
            keep.push_back(i);
            if (idxs.numel() == 1) break;

            torch::Tensor rest = idxs.slice(0, 1);
            auto b1 = boxes_cpu[i];
            auto area1 = (b1[2] - b1[0]) * (b1[3] - b1[1]);
            auto b2 = boxes_cpu.index_select(0, rest);
            auto xx1 = torch::max(b1[0], b2.select(1, 0));
            auto yy1 = torch::max(b1[1], b2.select(1, 1));
            auto xx2 = torch::min(b1[2], b2.select(1, 2));
            auto yy2 = torch::min(b1[3], b2.select(1, 3));
            auto w_int = torch::clamp(xx2 - xx1, 0);
            auto h_int = torch::clamp(yy2 - yy1, 0);
            auto inter = w_int * h_int;
            auto area2 = (b2.select(1, 2) - b2.select(1, 0)) *
                         (b2.select(1, 3) - b2.select(1, 1));
            auto ious = inter / (area1 + area2 - inter);

            auto mask_keep = ious <= iou_thresh;
            idxs = rest.index_select(0, mask_keep.nonzero().squeeze());
        }

        // === Construction des detections
        for (int id : keep) {
            auto box = xyxy[id];
            Detection det;
            det.x1 = box[0].item<float>() * frame.cols / 640.0f;
            det.y1 = box[1].item<float>() * frame.rows / 640.0f;
            det.x2 = box[2].item<float>() * frame.cols / 640.0f;
            det.y2 = box[3].item<float>() * frame.rows / 640.0f;
            det.conf = scores_sel[id].item<float>();
            det.cls = classes_sel[id].item<int>();
            results.push_back(det);
        }

        // === Dessin identique
        for (const auto& det : results) {
            cv::Scalar color;
            QString label;
            if (det.cls == 0) { color = cv::Scalar(0, 0, 255); label = "red"; }
            else if (det.cls == 1) { color = cv::Scalar(0, 255, 255); label = "yellow"; }
            else { color = cv::Scalar(192, 192, 192); label = "empty"; }

            cv::rectangle(frame,
                          cv::Point(det.x1, det.y1),
                          cv::Point(det.x2, det.y2),
                          color, 2);
            QString txt = QString("%1 (%.2f)").arg(label).arg(det.conf);
            cv::putText(frame, txt.toStdString(),
                        cv::Point(det.x1, det.y1 - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }

    } catch (const c10::Error& e) {
        qWarning() << "[AI] ❌ Erreur pendant l'inférence Torch:" << e.what();
    }

    return results;
}


QImage CameraAI::matToQImage(const cv::Mat& mat) {
    if (mat.empty()) return {};
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}
