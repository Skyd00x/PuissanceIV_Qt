#include "BoardDetector.hpp"

std::vector<cv::Vec3f> BoardDetector::workingCircles;

Board BoardDetector::detectBoard(cv::Mat image, Color playerColor)
{
	// Attente 0.5s
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// 1) Détection du rectangle de la grille
	cv::Rect boardRect = detectBoardRectangle(image);
	if (boardRect.area() == 0) {
		std::cout << "Grille non détectée !" << std::endl;
		return Board();
	}
	cv::rectangle(image, boardRect, cv::Scalar(0, 255, 0), 3);

	// 2) Détection des cercles dans le ROI
	std::vector<cv::Vec3f> circles = detectCirclesInROI(image, boardRect);
	if (circles.empty()) {
		std::cout << "Aucun cercle détecté dans la grille" << std::endl;
		return Board();
	}

	for (auto& c : circles) {
		cv::circle(image, cv::Point(c[0], c[1]), c[2], cv::Scalar(255, 255, 255), 2);
	}

	// 3) Pipeline normal
	cv::Vec3f firstCircle = searchFirstCircle(image, circles, playerColor);
	if (firstCircle == cv::Vec3f()) {
		std::cout << "No first circle detected" << std::endl;
		return Board();
	}
	cv::circle(image, cv::Point(firstCircle[0], firstCircle[1]), 5, cv::Scalar(255, 255, 255), -1);

	std::vector<cv::Vec3f> boardCircles = filterCircles(image, circles, firstCircle);
	std::vector<cv::Vec3f> sortedCircles = sortCircles(boardCircles);
	Board board = detectColors(image, sortedCircles);

	return board;
}

cv::Rect BoardDetector::detectBoardRectangle(cv::Mat& image)
{
	cv::Mat hsv, mask;
	cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

	// Bornes HSV pour le bleu (à ajuster selon ta lumière)
	cv::Scalar lower_blue(90, 100, 80);
	cv::Scalar upper_blue(130, 255, 255);

	cv::inRange(hsv, lower_blue, upper_blue, mask);

	// Nettoyage pour enlever le bruit
	cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::Mat(), cv::Point(-1, -1), 3);
	cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 1);
	cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	cv::Rect bestRect;
	double maxArea = 0.0;

	for (auto& contour : contours) {
		double area = cv::contourArea(contour);
		if (area < 20000) continue; // ignorer les petits objets

		cv::Rect rect = cv::boundingRect(contour);
		double ratio = (double)rect.width / (double)rect.height;

		// grille ~7 colonnes / 6 lignes => ratio ? 1.16
		if (ratio > 0.9 && ratio < 1.4) {
			if (area > maxArea) {
				maxArea = area;
				bestRect = rect;
			}
		}
	}

	return bestRect;
}


std::vector<cv::Vec3f> BoardDetector::detectCirclesInROI(cv::Mat& image, cv::Rect boardRect)
{
	std::vector<cv::Vec3f> circles;
	if (boardRect.area() == 0) return circles;

	cv::Mat roi = image(boardRect).clone();

	// Détection classique
	circles = detectCircle(roi);

	// Recalage des coordonnées
	for (auto& c : circles) {
		c[0] += boardRect.x;
		c[1] += boardRect.y;
	}

	return circles;
}

std::vector<cv::Vec3f> BoardDetector::detectCircle(cv::Mat frame)
{
	if (frame.empty())
	{
		return std::vector<cv::Vec3f>();
	}

	//Convert the frame to gray
	cv::Mat grayFrame;
	cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

	//Search for circles in the frame
	std::vector<cv::Vec3f> circles;

	//cv::HoughCircles(grayFrame, circles, cv::HOUGH_GRADIENT, 0.5, grayFrame.rows / 12, 200, 30, grayFrame.rows / 24, grayFrame.rows / 6);
	cv::HoughCircles(grayFrame, circles, cv::HOUGH_GRADIENT, 0.5, grayFrame.rows / 12, 125, 28, grayFrame.rows / 24, grayFrame.rows / 6);

	return circles;
}

cv::Vec3f BoardDetector::searchFirstCircle(cv::Mat image, std::vector<cv::Vec3f> circles, Color playerColor)
{
	if (circles.size() == 0)
	{
		return cv::Vec3f();
	}

	//Search the first circle with the player color inside
	uint closestCircleIndex = -1;
	uint closestCircleValue = 1000000;
	cv::Vec3f firstCircle = cv::Vec3f();

	std::cout << "circles.size: ***********************" << circles.size() << std::endl;
	for (int i = 0; i < circles.size(); i++)
	{
		cv::Vec3b detectedColor = getCircleMeanColor(image, circles[i]);

		Color detectedColorType = getColor(detectedColor);

		if (detectedColorType == Color::RED)
			std::cout << "the color is: RED!!" << std::endl;
		else if (detectedColorType == Color::YELLOW)
			std::cout << "the color is: YELLOW!!" << std::endl;
		else if (detectedColorType == Color::EMPTY)
			std::cout << "the color is: EMPTY!!" << std::endl;
		else
			std::cout << "Nothing" << std::endl;

		if (detectedColorType == Color::EMPTY)
		{
			continue;
		}
		if (detectedColorType == playerColor)
		{
			closestCircleIndex = i;
			firstCircle = circles[closestCircleIndex];
			//Remove the first circle from the circles vector
			circles.erase(circles.begin() + closestCircleIndex);
			break;
		}
	}



	std::cout << "circles.size: +++++++++++++++++++" << circles.size() << std::endl;
	return firstCircle;
}

std::vector<cv::Vec3f> BoardDetector::filterCircles(cv::Mat image, std::vector<cv::Vec3f> circles, cv::Vec3f firstCircle)
{
	std::vector<cv::Vec3f> boardCircles;

	std::cout << "circles.size: _________________" << circles.size() << std::endl;

	//Search in the same line
	uint i = 0;
	while (i < circles.size())
	{
		if (circles[i][1] >= firstCircle[1] - 10 && circles[i][1] <= firstCircle[1] + 10) //&& circles[i][2] >= firstCircle[2] - 5 && circles[i][2] <= firstCircle[2] + 5)
		{
			boardCircles.push_back(circles[i]);
			circles.erase(circles.begin() + i);
		}
		i++;
	}

	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ¼ì²âÎÊÌâ
	if (boardCircles.size() != 7)
	{
		std::cout << "Not enough circles in the same line:    " << boardCircles.size() << std::endl;
		return std::vector<cv::Vec3f>();
	}

	//Search in the same column
	for (int i = 0; i < boardCircles.size(); i++)
	{
		uint j = 0;
		while (j < circles.size())
		{
			if (circles[j][0] >= boardCircles[i][0] - 10 && circles[j][0] <= boardCircles[i][0] + 10 && circles[j][2] >= boardCircles[i][2] - 5 && circles[j][2] <= boardCircles[i][2] + 5)
			{
				boardCircles.push_back(circles[j]);
				circles.erase(circles.begin() + j);
			}
			j++;
		}
	}

	//If the board is not complete
	if (boardCircles.size() != 42)
	{
		std::cout << "Not enough circles in the same column" << std::endl;
		return std::vector<cv::Vec3f>();
	}

	return boardCircles;
}

std::vector<cv::Vec3f> BoardDetector::sortCircles(std::vector<cv::Vec3f> boardCircles)
{
	if (boardCircles.size() != 42)
	{
		return std::vector<cv::Vec3f>();
	}

	std::vector<cv::Vec3f> sortedCircles;

	//For all the columns
	for (uint i = 0; i < 7; i++)
	{
		uint min = 1000000;
		uint minIndex = 0;

		//Search the first circle at minimum x and y
		for (int j = 0; j < boardCircles.size(); j++)
		{
			if (boardCircles[j][0] + boardCircles[j][1] < min)
			{
				min = boardCircles[j][0] + boardCircles[j][1];
				minIndex = j;
			}
		}

		//Add the first circle to the sortedCircles vector
		sortedCircles.push_back(boardCircles[minIndex]);
		//Remove the first circle from the boardCircles vector
		boardCircles.erase(boardCircles.begin() + minIndex);

		//Search the next 5 circles in the same line
		for (int j = 0; j < 5; j++)
		{
			min = 1000000;
			minIndex = 0;
			for (int j = 0; j < boardCircles.size(); j++)
			{
				if (abs(boardCircles[j][0] - sortedCircles[i * 6][0]) < sortedCircles[i * 6][2] && boardCircles[j][1] < min)
				{
					min = boardCircles[j][1];
					minIndex = j;
				}
			}
			sortedCircles.push_back(boardCircles[minIndex]);
			boardCircles.erase(boardCircles.begin() + minIndex);
		}
	}

	return sortedCircles;
}

Board BoardDetector::detectColors(cv::Mat image, std::vector<cv::Vec3f> boardCircles)
{
	if (boardCircles.size() != 42)
	{
		if (workingCircles.size() == 42)
		{
			boardCircles = workingCircles;
		}
		else
		{
			return Board();
		}
	}
	else
	{
		workingCircles = boardCircles;
	}

	Board board;
	uint playerThreshold = 200;
	uint robotThreshold = 150;

	for (int i = 0; i < boardCircles.size(); i++)
	{
		cv::Vec3b pieceColor = getCircleMeanColor(image, boardCircles[i]);

		//Draw the circle with the detected color for debug
		cv::circle(image, cv::Point(boardCircles[i][0], boardCircles[i][1]), boardCircles[i][2] - 10, cv::Scalar(pieceColor[0], pieceColor[1], pieceColor[2]), -1);

		//Detect the color of the circle
		Color color = getColor(pieceColor);

		if (color == Color::EMPTY)
		{
			continue;
		}
		else if (color == Color::RED)
		{

			board.setPlayerPiece(i / 6, 5 - i % 6, true);
		}
		else
		{
			board.setRobotPiece(i / 6, 5 - i % 6, true);
		}
	}
	//Uncomment to show the debug image with the detected circles
	//cv::imshow("Debug", image);

	return board;
}

cv::Vec3b BoardDetector::getCircleMeanColor(cv::Mat image, cv::Vec3f circle)
{
	//Create a mask to get only the circle pixels
	cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
	cv::circle(mask, cv::Point(circle[0], circle[1]), circle[2] - 10, cv::Scalar(255), -1);

	//Get the mean color of the circle
	cv::Scalar mean = cv::mean(image, mask);
	return cv::Vec3b(mean[0], mean[1], mean[2]);
}

BoardDetector::Color BoardDetector::getColor(cv::Vec3b color)
{
	//if (abs(color[0] - color[1]) < 60 && abs(color[1] - color[2]) < 60 && abs(color[0] - color[2]) < 60)
	if (abs(color[0] - color[1]) < 80 && abs(color[1] - color[2]) < 80 && abs(color[0] - color[2]) < 80)
	{
		return Color::EMPTY;
	}
	if (abs(color[2] - color[0]) > 50 && abs(color[2] - color[1]) > 50)
	{
		return Color::RED;
	}
	return Color::YELLOW;
}

std::vector<cv::Vec3f> BoardDetector::addAndRemoveDuplicates(std::vector<cv::Vec3f> newCircles, std::vector<cv::Vec3f> oldCircles)
{
	std::vector<cv::Vec3f> result = newCircles;

	for (int i = 0; i < oldCircles.size(); i++)
	{
		result.push_back(oldCircles[i]);
	}

	for (int i = 0; i < result.size(); i++)
	{
		for (int j = i + 1; j < result.size(); j++)
		{
			if (abs(result[i][0] - result[j][0]) < 10 && abs(result[i][1] - result[j][1]) < 10)
			{
				result.erase(result.begin() + j);
			}
		}
	}

	return result;
}


