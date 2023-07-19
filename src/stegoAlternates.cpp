#include <iostream>
#include <opencv2/opencv.hpp>
#include <tuple>
#include "evaluationMetrics.h"

using namespace std;
using namespace cv;

struct EncodedImages {
    Mat secretImage;
    Mat stegoImage;
};


// Converts a byte (unsigned char) to an 8-bit binary string
string byteToBinary(unsigned char b) {
    string binary = "";
    for (int i = 7; i >= 0; i--) {
        binary += (b & (1 << i)) ? "1" : "0";
    }

    return binary;
}

// Converts a string of characters to a binary string
string stringToBinary(string input) {
    string binary = "";

    for (char& c : input) {
        binary += byteToBinary(c);
    }

    return binary;
}

int transformBitsToDecimal(string bits) {
    int ans = 0;
    int len = bits.length();

    for(int i = len - 1; i >= 0 ; i--) {
        ans += (pow(2, i) * (bits[len - i - 1] == '1' ? 1 : 0));
    }

    return ans;
}

int calculateFEMD(uchar pixel1, uchar pixel2) {
    if(pixel1 > 255 || pixel1 < 0 || pixel2 > 255 || pixel2 < 0) {
        return -1;
    }

    int lsbPixel1 = pixel1 & 1;
    int tempCalc = (pixel1/2) + pixel2;

    int femd1 = lsbPixel1 * 2;
    int femd2 = tempCalc & 1;
    
    return femd1 + femd2;
}

void binaryToString(string binaryString) {
    string res = "";

    for(int i = 0; i < binaryString.length(); i += 8) {
        int asciiVal = transformBitsToDecimal(binaryString.substr(i, 8));
        res += char(asciiVal);
    }

    cout << res << endl;
}


// Function to encode a message into a cover image using the LSB matching revisited method
EncodedImages encodeLSBMatchingRevisited(Mat &coverImage, string message)
{
    EncodedImages result = {};

    // Create a second image to hold the message
    Mat secretImage = coverImage.clone();

    // Clone the cover image
    Mat stegoImage = coverImage.clone();

    // Determine the number of pixels required to store the message
    int numPixels = message.length() * 8;

    // Determine the maximum message length that can be hidden in the cover image
    int maxMessageLength = (stegoImage.total() * stegoImage.channels() * 2);

    if (numPixels > maxMessageLength) {
        cerr << "Message too long to hide in cover image." << endl;
        exit(1);
    }

    // Convert the message to binary format
    string binaryMessage = stringToBinary(message);

    // Keep track over the index of the current message to be embedded
    int messageIndex = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            // Get the pixel value at the current location
            uchar pixel = stegoImage.at<uchar>(row, col);

            string binaryPixel = byteToBinary(pixel);
            string messageKey = "";

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            messageIndex += 2;

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            int key = transformBitsToDecimal(messageKey);
            
            if (key == calculateFEMD(pixel, pixel)) {
                secretImagePixel = pixel;
                stegoImagePixel = pixel;
            } else if (key == calculateFEMD(pixel , pixel + 1)) {
                secretImagePixel = pixel;
                stegoImagePixel = pixel + 1;
            } else if (key == calculateFEMD(pixel + 1, pixel - 1)) {
                secretImagePixel = pixel + 1;
                stegoImagePixel = pixel - 1;
            } else if (key == calculateFEMD(pixel + 1, pixel)) {
                secretImagePixel = pixel + 1;
                stegoImagePixel = pixel;
            }

            secretImage.at<uchar>(row, col) = secretImagePixel;
            stegoImage.at<uchar>(row, col) = stegoImagePixel;


            if(messageIndex >= binaryMessage.length()) {
                result.secretImage = secretImage;
                result.stegoImage = stegoImage;

                return result;
            }
            /*
            // Get the RGB values of the cover pixel and the secret pixel
             Vec3b coverPixel = coverImage.at<Vec3b>(row, col);
             Vec3b secretPixel = secretImage.at<Vec3b>(row, col);

            // Iterate over the channels (BGR)
            for (int channel = 0; channel < 3; channel++)
            {
                // Check if there are still message bits to be encoded
                if (pixelIndex < numMessageBits)
                {
                    // Get the least significant bit of the secret pixel channel
                    int secretBit = (secretPixel[channel] & 1);

                    // Modify the least significant bit of the cover pixel channel
                    if (coverPixel[channel] % 2 == 0)
                    {
                        // If the cover pixel channel is even, set the LSB to the secret bit
                        if (secretBit == 1)
                        {
                            coverPixel[channel]++;
                        }
                    }
                    else
                    {
                        // If the cover pixel channel is odd, unset the LSB if it doesn't match the secret bit
                        if (secretBit == 0)
                        {
                            coverPixel[channel]--;
                        }
                    }
                }

                // Increment the pixel index
                pixelIndex++;
            }

            // Set the modified cover pixel value
            coverImage.at<Vec3b>(row, col) = coverPixel;
            */
        }
        
    }

    result.secretImage = secretImage;
    result.stegoImage = stegoImage;

    return result;
}


// Function to decode a message from a stego image using the LSB matching revisited method
string decodeLSBMatchingRevisited(Mat &secretImage, Mat &stegoImage, int length)
{
    // Get the number of message bits to be decoded
    Mat originalImage = secretImage.clone();
    string binaryMessage = "";

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            if(binaryMessage.length() >= length) {
                testQuality(originalImage, secretImage);
                return binaryMessage;
            }
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(row, col);

            originalImage.at<uchar>(row, col) = (pixel1 + pixel2)/2;

            int lsbPixel1 = pixel1 & 1;
            int tempCalc = (pixel1/2) + pixel2;
            int femd2 = tempCalc & 1;

            binaryMessage += to_string(lsbPixel1) + to_string(femd2);
        }

        
    }

    return binaryMessage;
}

// Function to encode a message into a cover image using the LSB matching revisited method
EncodedImages encodeOption1(Mat &coverImage, Mat &secondaryImage, string message)
{
    EncodedImages result = {};

    // Create a second image to hold the message
    Mat secretImage = coverImage.clone();

    // Clone the cover image
    Mat stegoImage = secondaryImage.clone();

    // Determine the number of pixels required to store the message
    int numPixels = message.length() * 8;

    // Determine the maximum message length that can be hidden in the cover image
    int maxMessageLength = (stegoImage.total() * stegoImage.channels()) + (secretImage.total() * secretImage.channels());

    if (numPixels > maxMessageLength) {
        cerr << "Message too long to hide in cover image." << endl;
        exit(1);
    }

    // Convert the message to binary format
    string binaryMessage = stringToBinary(message);

    // Keep track over the index of the current message to be embedded
    int messageIndex = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(row, col);

            string messageKey = "";

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            messageIndex += 2;

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            int key = transformBitsToDecimal(messageKey);
            
            if (key == calculateFEMD(pixel1, pixel2)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2;
            } else if (key == calculateFEMD(pixel1 , pixel2 + 1)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2 + 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2 - 1)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2 - 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2;
            }

            string binaryPixel = byteToBinary(stegoImagePixel);
            binarPixel[binaryPixel.size() - 2] = pixel1 % 2;

            int transformedStegoImagePixel = transformBitsToDecimal(binarPixel);

            secretImage.at<uchar>(row, col) = secretImagePixel;
            stegoImage.at<uchar>(row, col) = transformedStegoImagePixel;

            if(messageIndex >= binaryMessage.length()) {
                result.secretImage = secretImage;
                result.stegoImage = stegoImage;

                return result;
            }
        }
        
    }

    result.secretImage = secretImage;
    result.stegoImage = stegoImage;

    return result;
}

// Function to decode a message from a stego image using the LSB matching revisited method
string decodeOption1(Mat &secretImage, Mat &stegoImage, int length)
{
    // Get the number of message bits to be decoded
    Mat originalImage = secretImage.clone();
    string binaryMessage = "";

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            if(binaryMessage.length() >= length) {
                testQuality(originalImage, secretImage);
                return binaryMessage;
            }
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(row, col);

            string binaryPixel = byteToBinary(pixel2);
            int isOriginalPixelEven = binaryPixel[binaryPixel.size() - 2] == "1" ? 1 : 0;
            int originalPixelVal;

            if(isOriginalPixelEven) {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 : pixel1 - 1;
            } else {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 - 1 : pixel1;
            }

            originalImage.at<uchar>(row, col) = originalPixelVal;

            int lsbPixel1 = pixel1 & 1;
            int tempCalc = (pixel1/2) + pixel2;
            int femd2 = tempCalc & 1;

            binaryMessage += to_string(lsbPixel1) + to_string(femd2);
        }

        
    }

    return binaryMessage;
}

// Function to encode a message into a cover image using the LSB matching revisited method
/* EncodedImages encodeOption2(Mat &coverImage, Mat &secondaryImage, string message)
{
    EncodedImages result = {};

    // Create a second image to hold the message
    Mat secretImage = coverImage.clone();

    // Clone the cover image
    Mat stegoImage = secondaryImage.clone();

    // Determine the number of pixels required to store the message
    int numPixels = message.length() * 8;

    // Determine the maximum message length that can be hidden in the cover image
    int maxMessageLength = (stegoImage.total() * stegoImage.channels()) + (secretImage.total() * secretImage.channels());

    if (numPixels > maxMessageLength) {
        cerr << "Message too long to hide in cover image." << endl;
        exit(1);
    }

    // Convert the message to binary format
    string binaryMessage = stringToBinary(message);

    // Keep track over the index of the current message to be embedded
    int messageIndex = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            // Get the pixel value at the current location
            uchar pixel = stegoImage.at<uchar>(row, col);

            string binaryPixel = byteToBinary(pixel);
            string messageKey = "";

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            messageIndex += 2;

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            int key = transformBitsToDecimal(messageKey);
            
            if (key == calculateFEMD(pixel, pixel)) {
                secretImagePixel = pixel;
                stegoImagePixel = pixel;
            } else if (key == calculateFEMD(pixel , pixel + 1)) {
                secretImagePixel = pixel;
                stegoImagePixel = pixel + 1;
            } else if (key == calculateFEMD(pixel + 1, pixel - 1)) {
                secretImagePixel = pixel + 1;
                stegoImagePixel = pixel - 1;
            } else if (key == calculateFEMD(pixel + 1, pixel)) {
                secretImagePixel = pixel + 1;
                stegoImagePixel = pixel;
            }

            secretImage.at<uchar>(row, col) = secretImagePixel;
            stegoImage.at<uchar>(row, col) = stegoImagePixel;


            if(messageIndex >= binaryMessage.length()) {
                result.secretImage = secretImage;
                result.stegoImage = stegoImage;

                return result;
            }
        }
        
    }

    result.secretImage = secretImage;
    result.stegoImage = stegoImage;

    return result;
} */

// Function to encode a message into a cover image using the LSB matching revisited method
EncodedImages encodeOption3(Mat &coverImage, Mat &secondaryImage, string message)
{
    EncodedImages result = {};

    // Create a second image to hold the message
    Mat secretImage = coverImage.clone();

    // Clone the cover image
    Mat stegoImage = secondaryImage.clone();

    // Determine the number of pixels required to store the message
    int numPixels = message.length() * 8;

    // Determine the maximum message length that can be hidden in the cover image
    int maxMessageLength = (stegoImage.total() * stegoImage.channels()) + (secretImage.total() * secretImage.channels());

    if (numPixels > maxMessageLength) {
        cerr << "Message too long to hide in cover image." << endl;
        exit(1);
    }

    // Convert the message to binary format
    string binaryMessage = stringToBinary(message);

    // Keep track over the index of the current message to be embedded
    int messageIndex = 0;

    int stegoImageRow = 0;
    int stegoImageCol = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol);

            string binaryPixel = byteToBinary(pixel);
            string messageKey = "";

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            messageIndex += 2;

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            int key = transformBitsToDecimal(messageKey);
            
            if (key == calculateFEMD(pixel1, pixel2)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2;
            } else if (key == calculateFEMD(pixel1 , pixel2 + 1)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2 + 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2 - 1)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2 - 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2;
            }

            string binaryPixel = byteToBinary(stegoImagePixel);
            binarPixel[binaryPixel.size() - 2] = pixel1 % 2;

            int transformedStegoImagePixel = transformBitsToDecimal(binarPixel);

            secretImage.at<uchar>(row, col) = secretImagePixel;
            stegoImage.at<uchar>(stegoImageRow, stegoImageCol) = stegoImagePixel;

            //Check the case when the number of pixels is odd
            if (col < cols - 1) {
                // Store the value besides the current one
                stegoImage.at<uchar>(stegoImageRow, stegoImageCol + 1) = transformedStegoImagePixel;
                stegoImageCol++;
            } else {
                // Check if there is a row besides the current one
                if (row < rows) {
                    // Store the value in the next row
                    stegoImageCol = 0;
                    stegoImage.at<uchar>(stegoImageRow + 1, stegoImageCol) = transformedStegoImagePixel;
                    stegoImageRow++;
                }
            }

            if(messageIndex >= binaryMessage.length()) {
                result.secretImage = secretImage;
                result.stegoImage = stegoImage;

                return result;
            }
        }
        
    }

    result.secretImage = secretImage;
    result.stegoImage = stegoImage;

    return result;
}

// Function to decode a message from a stego image using the LSB matching revisited method
string decodeOption3(Mat &secretImage, Mat &stegoImage)
{
    // Get the number of message bits to be decoded
    Mat originalImage = secretImage.clone();
    string binaryMessage = "";

    int stegoImageRow = 0;
    int stegoImageCol = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            if(binaryMessage.length() >= length) {
                testQuality(originalImage, secretImage);
                return binaryMessage;
            }
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol);

            uchar pixel3;

            if (col < cols - 1) {
                // Store the value besides the current one
                pixel3 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol + 1);
                stegoImageCol++;
            } else {
                // Check if there is a row besides the current one
                if (row < rows) {
                    // Store the value in the next row
                    stegoImageCol = 0;
                    pixel3 = stegoImage.at<uchar>(stegoImageRow + 1, stegoImageCol);
                    stegoImageRow++;
                }
            }

            int isOriginalPixelOdd = pixel3 & 1;
            int originalPixelVal;

            if(isOriginalPixelOdd) {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 - 1 : pixel1;
            } else {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 : pixel1 - 1;
            }

            originalImage.at<uchar>(row, col) = originalPixelVal;

            int lsbPixel1 = pixel1 & 1;
            int tempCalc = (pixel1/2) + pixel2;
            int femd2 = tempCalc & 1;

            binaryMessage += to_string(lsbPixel1) + to_string(femd2);
        }

        
    }

    return binaryMessage;
}    

// Function to encode a message into a cover image using the LSB matching revisited method
EncodedImages encodeOption4(vector<Mat>& images ,string message)
{
    EncodedImages result = {};
    vector<tuple<Mat, int, int>> clonedImages;
    int imagesLen = images.size();
    int maxMessageLength = 0;
    // int minImageRows = INT_MAX;
    // int minImageCols = INT_MAX;


    // Clone each image in the vector
    for (Mat& image : images) {
        // Create a clone of the image
        Mat clonedImage = image.clone();
        
        // Add the cloned image to the vector
        clonedImages.push_back(make_tuple(clonedImage, 0, 0));

        // minImageRows = min(minImageRows, clonedImage.rows);
        // minImageCols = min(minImageCols, clonedImage.cols);
        maxMessageLength += clonedImages.size() * clonedImges.channels();
    }

    // Determine the number of pixels required to store the message
    int numPixels = message.length() * 8;

    // maxMessageLength = minImageRows * minImageCols * clonedImages.size();

    if (numPixels > maxMessageLength) {
        cerr << "Message too long to hide in cover image." << endl;
        exit(1);
    }

    // Convert the message to binary format
    string binaryMessage = stringToBinary(message);

    // Keep track over the index of the current message to be embedded
    int messageIndex = 0;

    uchar prevImagePixel = get<0>(clonedImages[0]).at<uchar>(0, 0);
    uchar currImagePixel = get<0>(clonedImages[0]).at<uchar>(0, 0);

    while(messageIndex < numPixels) {
        for(int i = 0; i < imagesLen - 1; i += 2) {
            tuple<Mat, int, int> curr = &clonedImages[i];
            tuple<Mat, int, int> next = &clonedImages[i + 1];

            uchar pixel1 = get<0>(curr).at<uchar>(get<1>(curr), get<2>(curr));
            uchar pixel2 = get<0>(next).at<uchar>(get<1>(next), get<1>(next));

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            messageIndex += 2;

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            int key = transformBitsToDecimal(messageKey);
            
            if (key == calculateFEMD(pixel1, pixel2)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2;
            } else if (key == calculateFEMD(pixel1 , pixel2 + 1)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2 + 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2 - 1)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2 - 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2;
            }

            currImagePixel = pixel1;

            string binaryPixel1 = byteToBinary(secretImagePixel);
            binarPixel1[binaryPixel1.size() - 2] = prevImagePixel % 2;

            string binaryPixel2 = byteToBinary(stegoImagePixel);
            binarPixel2[binaryPixel2.size() - 2] = currImagePixel % 2;

            prevImagePixel = pixel2;

            int transformedStegoImagePixel1 = transformBitsToDecimal(binaryPixel1);
            int transformedStegoImagePixel2 = transformBitsToDecimal(binaryPixel2);

            get<0>(curr).at<uchar>(get<1>(curr), get<2>(curr)) = transformedStegoImagePixel1;
            get<0>(next).at<uchar>(get<1>(next), get<2>(next)) = transformedStegoImagePixel2;

            //Check the case when the number of pixels is odd
            if (get<2>(curr) < get<0>(curr).cols - 1) {
                // Store the value besides the current one
                get<2>(curr)++;
            } else {
                // Check if there is a row besides the current one
                if (get<1>(curr) < get<0>(curr).rows - 1) {
                    // Store the value in the next row
                    get<2>(curr) = 0;
                    get<1>(curr)++;
                }
            }

            //Check the case when the number of pixels is odd
            if (get<2>(next) < get<0>(next).cols - 1) {
                // Store the value besides the current one
                get<2>(next)++;
            } else {
                // Check if there is a row besides the current one
                if (get<1>(next) < get<0>(next).rows - 1) {
                    // Store the value in the next row
                    get<2>(next) = 0;
                    get<1>(next)++;
                }
            }
        }
    }

    return result;
}

// Function to decode a message from a stego image using the LSB matching revisited method
string decodeOption4(Mat &secretImage, Mat &stegoImage)
{
    // Get the number of message bits to be decoded
    Mat originalImage = secretImage.clone();
    string binaryMessage = "";

    int stegoImageRow = 0;
    int stegoImageCol = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            if(binaryMessage.length() >= length) {
                testQuality(originalImage, secretImage);
                return binaryMessage;
            }
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol);

            uchar pixel3;

            if (col < cols - 1) {
                // Store the value besides the current one
                pixel3 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol + 1);
                stegoImageCol++;
            } else {
                // Check if there is a row besides the current one
                if (row < rows) {
                    // Store the value in the next row
                    stegoImageCol = 0;
                    pixel3 = stegoImage.at<uchar>(stegoImageRow + 1, stegoImageCol);
                    stegoImageRow++;
                }
            }

            int isOriginalPixelOdd = pixel3 & 1;
            int originalPixelVal;

            if(isOriginalPixelOdd) {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 - 1 : pixel1;
            } else {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 : pixel1 - 1;
            }

            originalImage.at<uchar>(row, col) = originalPixelVal;

            int lsbPixel1 = pixel1 & 1;
            int tempCalc = (pixel1/2) + pixel2;
            int femd2 = tempCalc & 1;

            binaryMessage += to_string(lsbPixel1) + to_string(femd2);
        }

        
    }

    return binaryMessage;
}    

// Function to encode a message into a cover image using the LSB matching revisited method
EncodedImages encodeOption5(Mat &coverImage, Mat &secondaryImage, string message)
{
    EncodedImages result = {};

    // Create a second image to hold the message
    Mat secretImage = coverImage.clone();

    // Clone the cover image
    Mat stegoImage = secondaryImage.clone();

    // Determine the number of pixels required to store the message
    int numPixels = message.length() * 8;

    // Determine the maximum message length that can be hidden in the cover image
    int maxMessageLength = (stegoImage.total() * stegoImage.channels()) + (secretImage.total() * secretImage.channels());

    if (numPixels > maxMessageLength) {
        cerr << "Message too long to hide in cover image." << endl;
        exit(1);
    }

    // Convert the message to binary format
    string binaryMessage = stringToBinary(message);

    // Keep track over the index of the current message to be embedded
    int messageIndex = 0;

    int stegoImageRow = 0;
    int stegoImageCol = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol);

            string binaryPixel = byteToBinary(pixel);
            string messageKey = "";

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            messageIndex += 2;

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            int key = transformBitsToDecimal(messageKey);
            
            if (key == calculateFEMD(pixel1, pixel2)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2;
            } else if (key == calculateFEMD(pixel1 , pixel2 + 1)) {
                secretImagePixel = pixel1;
                stegoImagePixel = pixel2 + 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2 - 1)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2 - 1;
            } else if (key == calculateFEMD(pixel1 + 1, pixel2)) {
                secretImagePixel = pixel1 + 1;
                stegoImagePixel = pixel2;
            }

            string binaryPixel = byteToBinary(stegoImagePixel);
            binarPixel[binaryPixel.size() - 2] = pixel1 % 2;

            int transformedStegoImagePixel = transformBitsToDecimal(binarPixel);

            secretImage.at<uchar>(row, col) = secretImagePixel;
            stegoImage.at<uchar>(stegoImageRow, stegoImageCol) = stegoImagePixel;

            //Check the case when the number of pixels is odd
            if (col < cols - 1) {
                // Store the value besides the current one
                stegoImage.at<uchar>(stegoImageRow, stegoImageCol + 1) = transformedStegoImagePixel;
                stegoImageCol++;
            } else {
                // Check if there is a row besides the current one
                if (row < rows) {
                    // Store the value in the next row
                    stegoImageCol = 0;
                    stegoImage.at<uchar>(stegoImageRow + 1, stegoImageCol) = transformedStegoImagePixel;
                    stegoImageRow++;
                }
            }

            if(messageIndex >= binaryMessage.length()) {
                result.secretImage = secretImage;
                result.stegoImage = stegoImage;

                return result;
            }
        }
        
    }

    result.secretImage = secretImage;
    result.stegoImage = stegoImage;

    return result;
}

// Function to decode a message from a stego image using the LSB matching revisited method
string decodeOption5(Mat &secretImage, Mat &stegoImage)
{
    // Get the number of message bits to be decoded
    Mat originalImage = secretImage.clone();
    string binaryMessage = "";

    int stegoImageRow = 0;
    int stegoImageCol = 0;

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            if(binaryMessage.length() >= length) {
                testQuality(originalImage, secretImage);
                return binaryMessage;
            }
            // Get the pixel value at the current location
            uchar pixel1 = secretImage.at<uchar>(row, col);
            uchar pixel2 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol);

            uchar pixel3;

            if (col < cols - 1) {
                // Store the value besides the current one
                pixel3 = stegoImage.at<uchar>(stegoImageRow, stegoImageCol + 1);
                stegoImageCol++;
            } else {
                // Check if there is a row besides the current one
                if (row < rows) {
                    // Store the value in the next row
                    stegoImageCol = 0;
                    pixel3 = stegoImage.at<uchar>(stegoImageRow + 1, stegoImageCol);
                    stegoImageRow++;
                }
            }

            int isOriginalPixelOdd = pixel3 & 1;
            int originalPixelVal;

            if(isOriginalPixelOdd) {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 - 1 : pixel1;
            } else {
                originalPixelVal = (pixe1 % 2) == 0 ? pixel1 : pixel1 - 1;
            }

            originalImage.at<uchar>(row, col) = originalPixelVal;

            int lsbPixel1 = pixel1 & 1;
            int tempCalc = (pixel1/2) + pixel2;
            int femd2 = tempCalc & 1;

            binaryMessage += to_string(lsbPixel1) + to_string(femd2);
        }

        
    }

    return binaryMessage;
}    


/*
    // Iterate over the pixels of the stego image
    int pixelIndex = 0;
    string decodedMessage;
    for (int row = 0; row < stegoImage.rows; row++)
    {
        for (int col = 0; col < stegoImage.cols; col++)
        {
            // Get the RGB values of the stego pixel
            Vec3b stegoPixel = stegoImage.at<Vec3b>(row, col);

            // Iterate over the channels (BGR)
            for (int channel = 0; channel < 3; channel++)
            {
                // Check if there are still message bits to be decoded
                if (pixelIndex < numMessageBits)
                {
                    // Get the least significant bit of the stego pixel channel
                    int stegoBit = (stegoPixel[channel] & 1);

                    // Append the stego bit to the decoded message
                    decodedMessage.push_back(stegoBit + '0');
                }

                // Increment the pixel index
                pixelIndex++;
            }
        }
    }

    // Convert the binary string to a character string
    string decodedString;
    for (int i = 0; i < decodedMessage.size(); i += 8)
    {
        char c = 0;
        for (int j = 0; j < 8; j++)
        {
            c = (c << 1) | (decodedMessage[i + j] - '0');
        }
        decodedString.push_back(c);
    }

    return decodedString;
}

// Function to restore the cover image from a secret image and a stego image
Mat restoreCoverImage(Mat &secretImage, Mat &stegoImage)
{
    // Check that the dimensions of the secret and stego images are the same
    if (secretImage.rows != stegoImage.rows || secretImage.cols != stegoImage.cols)
    {
        cout << "Error: The dimensions of the secret and stego images are not the same." << endl;
        return Mat();
    }

    // Create a copy of the stego image
    Mat coverImage = stegoImage.clone();

    // Iterate over the pixels of the secret image
    int pixelIndex = 0;
    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            // Get the RGB values of the secret pixel
            Vec3b secretPixel = secretImage.at<Vec3b>(row, col);

            // Iterate over the channels (BGR)
            for (int channel = 0; channel < 3; channel++)
            {
                // Get the least significant bit of the cover pixel channel
                int coverBit = (coverImage.at<Vec3b>(row, col)[channel] & 1);

                // Get the least significant bit of the secret pixel channel
                int secretBit = (secretPixel[channel] & 1);

                // Set the least significant bit of the cover pixel channel to the secret bit
                if (coverBit != secretBit)
                {
                    coverImage.at<Vec3b>(row, col)[channel] ^= 1;
                }

                // Increment the pixel index
                pixelIndex++;
            }
        }
    }

    return coverImage;
}
*/

// Function to encode a message into a cover image using the LSB matching revisited method
vector<Mat> encodeOption5(vector<Mat>& images ,string message)
{
    vector<vector<int>> imageIndexes;
    vector<Mat> clonedImages;
    int maxMessageLength = 0;
    // int minImageRows = INT_MAX;
    // int minImageCols = INT_MAX;

    int clonedImagesIdx = 0;

    // Clone each image in the vector
    for (Mat& image : images) {
        // Create a clone of the image
        Mat clonedImage = image.clone();
        
        // Add the cloned image to the vector
        clonedImages.push_back(image);

        vector<int> item = {0, 0};
        imageIndexes.push_back(item);

        if((clonedImagesIdx + 1) % 3 == 0) {
            // Create a clone of the image
            Mat dualImageClone = image.clone();
            vector<int> dualImageItem = {0, 0};
            
            // Add the cloned image to the vector
            clonedImages.push_back(dualImageClone);
            imageIndexes.push_back(item);
        }

        maxMessageLength += clonedImage.total() * clonedImage.channels();
        clonedImagesIdx++;
    }

    // Determine the number of pixels required to store the message
    int numPixels = message.length() * 8;

    // maxMessageLength = minImageRows * minImageCols * clonedImages.size();

    if (numPixels > maxMessageLength) {
        cerr << "Message too long to hide in cover image." << endl;
        exit(1);
    }

    // Convert the message to binary format
    string binaryMessage = stringToBinary(message);

    string encryptedMessage = "";

    // Keep track over the index of the current message to be embedded

    for(int messageIndex = 0; messageIndex < numPixels; messageIndex += 2) {
        for(int i = 0; i < clonedImages.size() - 3; i += 4) {
            string messageKey = "";

            uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
            uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);
            uchar pixel3 = clonedImages[i + 2].at<uchar>(imageIndexes[i + 2][0], imageIndexes[i + 2][1]);
            uchar pixel4 = clonedImages[i + 3].at<uchar>(imageIndexes[i + 3][0], imageIndexes[i + 3][1]);

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            
            int key = transformBitsToDecimal(messageKey);

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            tie(secretImagePixel, stegoImagePixel) = checkFEMDCases(pixel1, pixel2, key);

            string binaryPixel2 = byteToBinary(stegoImagePixel);

            binaryPixel2[binaryPixel2.size() - 1] = pixel1 % 2;

            int transformedStegoImagePixel2 = transformBitsToDecimal(binaryPixel2);


            clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = secretImagePixel;
            clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]) = transformedStegoImagePixel2;

            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);


            uchar clonedImagePixel1 = 0;
            uchar clonedImagePixel2 = 0;

            string clonedImageMessage = "";
            clonedImageMessage += to_string(stegoImagePixel & 1);
            clonedImageMessage += to_string(pixel2 % 2);

            int clonedImageKey = transformBitsToDecimal(clonedImageMessage);
            tie(clonedImagePixel1, clonedImagePixel2) = checkFEMDCases(pixel3, pixel4, clonedImageKey);
            

            
            clonedImages[i + 2].at<uchar>(imageIndexes[i + 2][0], imageIndexes[i + 2][1]) = clonedImagePixel1;
            clonedImages[i + 3].at<uchar>(imageIndexes[i + 3][0], imageIndexes[i + 3][1]) = clonedImagePixel2;

            moveIndexForward(imageIndexes[i + 2], clonedImages[i + 2].rows, clonedImages[i + 2].cols);
            moveIndexForward(imageIndexes[i + 3], clonedImages[i + 3].rows, clonedImages[i + 3].cols);
        }
    }

    return clonedImages;
}    

// Function to decode a message from a stego image using the LSB matching revisited method
string decodeOption5(vector<Mat>& images, int length)
{
    cout << "decryption" << endl;
    vector<vector<int>> imageIndexes;
    vector<Mat> clonedImages;

    // Clone each image in the vector
    for (int i = 0; i < images.size(); i++) {
        Mat image = images[i].clone();
        // Add the cloned image to the vector
        vector<int> item = {0, 0};

        imageIndexes.push_back(item);
        clonedImages.push_back(image);
    }

    // Get the number of message bits to be decoded
    string decodedMessage = "";

    while(decodedMessage.size() != length) {
        for(int i = 0; i < images.size() - 3; i += 4) {
            // Get the pixel value at the current location
            uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
            uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);
            uchar pixel3 = clonedImages[i + 2].at<uchar>(imageIndexes[i + 2][0], imageIndexes[i + 2][1]);
            uchar pixel4 = clonedImages[i + 3].at<uchar>(imageIndexes[i + 3][0], imageIndexes[i + 3][1]);

            int isOriginalImage1PixelEven = pixel2 & 1;

            int clonedDualImageMessage = calculateFEMD(pixel3, pixel4);
            string pixel2Info = byteToBinary(clonedDualImageMessage);

            int isOriginalImage2PixelEven = pixel2Info[pixel2Info.size() - 1] == '1' ? 1 : 0;

            string binaryPixel2 = byteToBinary(pixel2);
            binaryPixel2[binaryPixel2.size() - 1] = pixel2Info[pixel2Info.size() - 2];

            uchar modifiedPixel2 = transformBitsToDecimal(binaryPixel2);

            int message = calculateFEMD(pixel1, modifiedPixel2);
            string binaryMessage = byteToBinary(message);

            decodedMessage += binaryMessage[binaryMessage.size() - 2];
            decodedMessage += binaryMessage[binaryMessage.size() - 1];

            uchar originalPixel1;
            uchar originalPixel2;

            cout << to_string(pixel1) + " " + to_string(modifiedPixel2) << endl;

            tie(originalPixel1, originalPixel2) = getOriginalPixel(pixel1, modifiedPixel2, isOriginalImage1PixelEven, isOriginalImage2PixelEven);
            
            clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = originalPixel1;
            clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]) = originalPixel2;

            uchar clonedDualImagePixelOriginal = (pixel3 + pixel4) / 2;

            clonedImages[i + 2].at<uchar>(imageIndexes[i + 2][0], imageIndexes[i + 2][1]) = clonedDualImagePixelOriginal;
            clonedImages[i + 3].at<uchar>(imageIndexes[i + 3][0], imageIndexes[i + 3][1]) = clonedDualImagePixelOriginal;

            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);
            moveIndexForward(imageIndexes[i + 2], clonedImages[i + 2].rows, clonedImages[i + 2].cols);
            moveIndexForward(imageIndexes[i + 3], clonedImages[i + 3].rows, clonedImages[i + 3].cols);
        }
    }

    // testQuality(originalImage, secretImage);
    return decodedMessage;
}    



int main()
{
    // Load the cover image and the message
    Mat coverImage = imread("../images/test_image_1.jpeg", IMREAD_GRAYSCALE);

    if (coverImage.empty()) {
        cerr << "Failed to read the cover image." << endl;
        return -1;
    }

    imshow("Cover Image", coverImage);
    waitKey(0);

    // Define the secret message to be hidden
    string secretMessage = "This is a secret message.";

    // Call the function to hide the secret message in the cover image
    EncodedImages result = encodeLSBMatchingRevisited(coverImage, secretMessage);

    // Display the stego image
    imshow("Secret Image", result.secretImage);
    waitKey(0);

    imshow("Stego Image", result.stegoImage);
    waitKey(0);

    testQuality(coverImage, result.secretImage);
    testQuality(coverImage, result.stegoImage);

    string binaryMessage = stringToBinary(secretMessage);

    // Call the function to extract the secret message from the stego image
    string extractedMessage = decodeLSBMatchingRevisited(result.secretImage, result.stegoImage, binaryMessage.length());

    // Display the extracted secret message
    cout << "Extracted Message: " << extractedMessage << endl;
    cout << extractedMessage.compare(binaryMessage) << endl;

    binaryToString(extractedMessage);

    calculateVarianceMetric(coverImage);

    // Call the function to restore the cover image from the stego image
    // Mat restoredImage = restoreCoverImage(result.secretImage, result.stegoImage);

    // Display the restored cover image
    // imshow("Restored Image", restoredImage);
    // waitKey(0);

    return 0;
}