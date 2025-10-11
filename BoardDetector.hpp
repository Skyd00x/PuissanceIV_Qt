#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include "Board.hpp"

class BoardDetector {

public:
	enum class Color
	{
		RED,
		YELLOW,
		EMPTY
	};

	static Board detectBoard(cv::Mat image, Color playerColor);


private:

	/// <summary>
	/// Search for circles in a given image using the HoughCircles() function from openCV
	/// </summary>
	/// <param name="image">Source image containing the board</param>
	/// <returns>A vector containing all the circles detected in the image</returns>
	static std::vector<cv::Vec3f> detectCircle(cv::Mat image);

	/// <summary>
	/// Because the player start, the first played piece is searched
	/// </summary>
	/// <param name="image">Source image containing the board</param>
	/// <param name="circles">Vector containing all the circles detected in the image</param>
	/// <param name="playerColor">Color of the player</param>
	/// <returns>A vector containing the coordinate of the circle and its radius</returns>
	static cv::Vec3f searchFirstCircle(cv::Mat image, std::vector<cv::Vec3f> circles, Color playerColor);

	/// <summary>
	/// Filter the circles to only keep the one from the real board using their position
	/// </summary>
	/// <param name="image">Source image containing the board</param>
	/// <param name="circles">Vector containing all the circles detected in the image</param>
	/// <param name="firstCircle">Coordinates of the first circle played by the player</param>
	/// <returns>A vector containing the 42 coordinate of the circle from the board</returns>
	static std::vector<cv::Vec3f> filterCircles(cv::Mat image, std::vector<cv::Vec3f> circles, cv::Vec3f firstCircle);

	/// <summary>
	/// Sort the circles from the board in the correct order :
	/// 
	/// 0 6 12 18 24 30 36
	/// 1 7 13 19 25 31 37
	/// 2 8 14 20 26 32 38
	/// 3 9 15 21 27 33 39
	/// 4 10 16 22 28 34 40
	/// 5 11 17 23 29 35 41
	/// 
	/// </summary>
	/// <param name="boardCircles">Vector containing the circles from the board</param>
	/// <returns>A vector containing the circles from the boars in the correct order</returns>
	static std::vector<cv::Vec3f> sortCircles(std::vector<cv::Vec3f> boardCircles);

	/// <summary>
	/// Detect the color inside the circle acording to the image
	/// </summary>
	/// <param name="image">Source image containing the board</param>
	/// <param name="boardCircles">Vector containing the circles from the board and in a correct order</param>
	/// <returns>A board object describing the status of the game</returns>
	static Board detectColors(cv::Mat image, std::vector<cv::Vec3f> boardCircles);

	/// <summary>
	/// Get the color of the circle
	/// </summary>
	/// <param name="image">Image containing the circle</param>
	/// <param name="circle">Coordinates of the circle (x, y, radius)</param>
	/// <returns>RGB color inside the circle</returns>
	static cv::Vec3b getCircleMeanColor(cv::Mat image, cv::Vec3f circle);

	/// <summary>
	/// Get the color enum element corresponding to the given RGB color
	/// </summary>
	/// <param name="color">RGB color</param>
	/// <returns>Color enum element</returns>
	static Color getColor(cv::Vec3b color);

	static std::vector<cv::Vec3f> workingCircles;

	static std::vector<cv::Vec3f> addAndRemoveDuplicates(std::vector<cv::Vec3f> newCircles, std::vector<cv::Vec3f> previousCircles);

	static cv::Rect detectBoardRectangle(cv::Mat& image);

	static std::vector<cv::Vec3f> detectCirclesInROI(cv::Mat& image, cv::Rect boardRect);

};