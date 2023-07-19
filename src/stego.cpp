#include <iostream>
#include <opencv2/opencv.hpp>
#include "evaluationMetrics.h"

using namespace std;
using namespace cv;
using namespace std::chrono;

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

string binaryToString(string binaryString) {
    string res = "";

    for(int i = 0; i < binaryString.length(); i += 8) {
        int asciiVal = transformBitsToDecimal(binaryString.substr(i, 8));
        res += char(asciiVal);
    }

    return res;
}

string convertToBinary(int number) {
    string binary;

    // Convert to binary representation
    while (number > 0) {
        binary = to_string(number % 2) + binary;
        number /= 2;
    }

    return binary;
}

tuple<uchar, uchar> checkFEMDCases(uchar pixel1, uchar pixel2, int key) {

    uchar secretImagePixel = 0;
    uchar stegoImagePixel = 0;

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

    return make_tuple(secretImagePixel, stegoImagePixel);
}

void moveIndexForward(vector<int>& data, int rows, int cols) {
    //Check the case when the number of pixels is odd
    if (data[1] < cols - 1) {
        // Store the value besides the current one
        data[1]++;
    } else {
        // Check if there is a row besides the current one
        if (data[0] < rows - 1) {
            // Store the value in the next row
            data[1] = 0;
            data[0]++;
        }
    }
}

tuple<int, int> getOriginalPixel(uchar pixel1, uchar pixel2, int isEvenPixel1, int isEvenPixel2) {
    tuple<int, int> pair1 = make_tuple(pixel1, pixel2);
    tuple<int, int> pair2 = make_tuple(pixel1, pixel2 - 1);
    tuple<int, int> pair3 = make_tuple(pixel1 - 1, pixel2 + 1);
    tuple<int, int> pair4 = make_tuple(pixel1 - 1, pixel2);

    vector<tuple<int, int>> pairs;
    pairs.push_back(pair1);
    pairs.push_back(pair2);
    pairs.push_back(pair3);
    pairs.push_back(pair4);

    for(tuple<int, int> pair: pairs) {
        if((pixel1 % 2 == isEvenPixel1) && (pixel2 % 2 == isEvenPixel2)) {
            return pair;
        }
    }

    return make_tuple(0, 0);
}

void encodeHeader(vector<Mat>& images, vector<vector<int>>& imageIndexes, string binaryMessage, int selectedMode) {
    string binaryLength = convertToBinary(binaryMessage.size());
    // Pad with zeros if necessary
    while (binaryLength.size() < 24) {
        binaryLength = '0' + binaryLength;
    }

    cout << to_string(binaryMessage.size()) << endl;

    string binaryMode = convertToBinary(2);

    string binaryHeader = binaryMode + binaryLength;

    cout << binaryHeader << endl;
    cout << to_string(binaryHeader.size()) << endl;



    for(int i = 0; i < 26; i += 2) {
        for(int j = 0; j < images.size() - 1; j += 2) {
            uchar pixel1 = images[j].at<uchar>(imageIndexes[j][0], imageIndexes[j][1]);
            uchar pixel2 = images[j + 1].at<uchar>(imageIndexes[j + 1][0], imageIndexes[j + 1][1]);

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;

            string messageKey = "";
            messageKey += binaryHeader[i];
            messageKey += binaryHeader[i + 1];

            int key = transformBitsToDecimal(messageKey);

            tie(secretImagePixel, stegoImagePixel) = checkFEMDCases(pixel1, pixel2, key);

            
            images[j].at<uchar>(imageIndexes[j][0], imageIndexes[j][1]) = secretImagePixel;
            images[j + 1].at<uchar>(imageIndexes[j + 1][0], imageIndexes[j + 1][1]) = stegoImagePixel;

            moveIndexForward(imageIndexes[j], images[j].rows, images[j].cols);
            moveIndexForward(imageIndexes[j + 1], images[j + 1].rows, images[j + 1].cols);

            uchar pixel4 = images[j + 1].at<uchar>(imageIndexes[j + 1][0], imageIndexes[j + 1][1]);

            string binaryPixel4 = byteToBinary(pixel4);
            binaryPixel4[binaryPixel4.size() - 1] = pixel1 % 2;

            uchar clonedImagePixel2 = transformBitsToDecimal(binaryPixel4);
            
            images[j + 1].at<uchar>(imageIndexes[j + 1][0], imageIndexes[j + 1][1]) = clonedImagePixel2;

            moveIndexForward(imageIndexes[j], images[j].rows, images[j].cols);
            moveIndexForward(imageIndexes[j + 1], images[j + 1].rows, images[j + 1].cols);
        }
    }
}

void encodeHeaderOption1(vector<Mat>& images, vector<vector<int>>& imageIndexes, string binaryMessage, int selectedMode) {
    string binaryLength = convertToBinary(binaryMessage.size());
    // Pad with zeros if necessary
    while (binaryLength.size() < 24) {
        binaryLength = '0' + binaryLength;
    }

    cout << to_string(binaryMessage.size()) << endl;

    string binaryMode = convertToBinary(1);

    while (binaryMode.size() < 2) {
        binaryMode = '0' + binaryMode;
    }

    string binaryHeader = binaryMode + binaryLength;

    cout << binaryHeader << endl;
    cout << to_string(binaryHeader.size()) << endl;

    for(int i = 0; i < 26; i++) {
        for(int j = 0; j < images.size() - 1; j += 2) {
            uchar pixel1 = images[j].at<uchar>(imageIndexes[j][0], imageIndexes[j][1]);
            uchar pixel2 = images[j + 1].at<uchar>(imageIndexes[j + 1][0], imageIndexes[j + 1][1]);

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;

            string messageKey = "";
            messageKey += binaryHeader[i];
            messageKey += to_string(pixel1 % 2);

            int key = transformBitsToDecimal(messageKey);

            tie(secretImagePixel, stegoImagePixel) = checkFEMDCases(pixel1, pixel2, key);
            
            images[j].at<uchar>(imageIndexes[j][0], imageIndexes[j][1]) = secretImagePixel;
            images[j + 1].at<uchar>(imageIndexes[j + 1][0], imageIndexes[j + 1][1]) = stegoImagePixel;

            moveIndexForward(imageIndexes[j], images[j].rows, images[j].cols);
            moveIndexForward(imageIndexes[j + 1], images[j + 1].rows, images[j + 1].cols);
        }
    }
}


// Function to encode a message into a cover image using the LSB matching revisited method
vector<Mat> encodeLSBMatchingRevisited(Mat &coverImage, string message)
{
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
                vector<Mat> result;
                result.push_back(secretImage);
                result.push_back(stegoImage);

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

    vector<Mat> result;
    result.push_back(secretImage);
    result.push_back(stegoImage);

    return result;
}


// Function to decode a message from a stego image using the LSB matching revisited method
tuple<string, vector<Mat>> decodeLSBMatchingRevisited(vector<Mat>& images, int length)
{
    // Get the number of message bits to be decoded
    Mat secretImage = images[0];
    Mat stegoImage = images[1];
    Mat originalImage = images[0].clone();
    vector<Mat> clonedImages;
    string binaryMessage = "";

    for (int row = 0; row < secretImage.rows; row++)
    {
        for (int col = 0; col < secretImage.cols; col++)
        {
            if(binaryMessage.length() >= length) {
                clonedImages.push_back(originalImage);
                return make_tuple(binaryMessage, clonedImages);
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

    clonedImages.push_back(originalImage);
    return make_tuple(binaryMessage, clonedImages);
}

// Function to encode a message into a cover image using the LSB matching revisited method
vector<Mat> encodeOption1(vector<Mat>& images ,string message)
{
    vector<vector<int>> imageIndexes;
    vector<Mat> clonedImages;
    int maxMessageLength = 0;
    // int minImageRows = INT_MAX;
    // int minImageCols = INT_MAX;

    // Clone each image in the vector
    for (Mat& image : images) {
        // Create a clone of the image
        Mat clonedImage = image.clone();
        
        // Add the cloned image to the vector
        clonedImages.push_back(clonedImage);

        vector<int> item = {0, 0};
        imageIndexes.push_back(item);

        maxMessageLength += clonedImage.total() * clonedImage.channels();
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

    // Without header
    // encodeHeaderOption1(clonedImages, imageIndexes, binaryMessage, 1);

    string encryptedMessage = "";

    // Keep track over the index of the current message to be embedded

    for(int messageIndex = 0; messageIndex < numPixels; messageIndex++) {
        for(int i = 0; i < clonedImages.size() - 1; i += 2) {
            string messageKey = "";

            uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
            uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);
            
            messageKey += binaryMessage[messageIndex];
            messageKey += to_string(pixel1 % 2);
            
            int key = transformBitsToDecimal(messageKey);

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            tie(secretImagePixel, stegoImagePixel) = checkFEMDCases(pixel1, pixel2, key);

            clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = secretImagePixel;
            clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]) = stegoImagePixel;

            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);
        }
    }

    return clonedImages;
}    

// Function to decode a message from a stego image using the LSB matching revisited method
tuple<string, vector<Mat>> decodeOption1(vector<Mat>& images, int length)
{
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

    // Start
    // string decodedHeader = "";

    // while(decodedHeader.size() != 26) {
    //     for(int i = 0; i < images.size() - 1; i += 2) {
    //         uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
    //         uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);

    //         int message = calculateFEMD(pixel1, pixel2);
    //         string binaryMessage = byteToBinary(message);

    //         decodedHeader += binaryMessage[binaryMessage.size() - 2];
    //         int isOriginalImage1PixelEven = binaryMessage[binaryMessage.size() - 1] == '0' ? 0 : 1;

    //         uchar originalPixel1;
    //         uchar originalPixel2;

    //         tie(originalPixel1, originalPixel2) = getOriginalPixel(pixel1, pixel2, isOriginalImage1PixelEven, 0);
            
    //         clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = originalPixel1;

    //         moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
    //         moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);
    //     }
    // }

    // cout << decodedHeader << endl;

    // string modeString = decodedHeader.substr(0, 2);
    // string binaryMessage = decodedHeader.substr(2);

    // int mode = transformBitsToDecimal(modeString);
    // int binaryMessageLength = transformBitsToDecimal(binaryMessage);

    // cout << to_string(mode) << endl;
    // cout << to_string(binaryMessageLength) << endl;

    // End

    while(decodedMessage.size() != length) {
        for(int i = 0; i < images.size() - 1; i += 2) {
            // Get the pixel value at the current location
            uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
            uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);

            int message = calculateFEMD(pixel1, pixel2);
            string binaryMessage = byteToBinary(message);

            decodedMessage += binaryMessage[binaryMessage.size() - 2];
            int isOriginalImage1PixelEven = binaryMessage[binaryMessage.size() - 1] == '0' ? 0 : 1;

            uchar originalPixel1;
            uchar originalPixel2;

            tie(originalPixel1, originalPixel2) = getOriginalPixel(pixel1, pixel2, isOriginalImage1PixelEven, 0);
            
            clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = originalPixel1;

            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);
        }
    }

    // testQuality(originalImage, secretImage);
    return make_tuple(decodedMessage, clonedImages);
}    

// Function to encode a message into a cover image using the LSB matching revisited method
vector<Mat> encodeOption2(vector<Mat>& images ,string message)
{
    vector<vector<int>> imageIndexes;
    vector<Mat> clonedImages;
    int maxMessageLength = 0;
    // int minImageRows = INT_MAX;
    // int minImageCols = INT_MAX;

    // Clone each image in the vector
    for (Mat& image : images) {
        // Create a clone of the image
        Mat clonedImage = image.clone();
        
        // Add the cloned image to the vector
        clonedImages.push_back(clonedImage);

        vector<int> item = {0, 0};
        imageIndexes.push_back(item);

        maxMessageLength += clonedImage.total() * clonedImage.channels();
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

    

    // encodeHeader(clonedImages, imageIndexes, binaryMessage, 2);

    string encryptedMessage = "";

    // Keep track over the index of the current message to be embedded

    for(int messageIndex = 0; messageIndex < numPixels; messageIndex += 2) {
        for(int i = 0; i < clonedImages.size() - 1; i += 2) {
            string messageKey = "";

            uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
            uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);

            messageKey += binaryMessage[messageIndex];
            messageKey += binaryMessage[messageIndex + 1];
            
            int key = transformBitsToDecimal(messageKey);

            uchar secretImagePixel = 0;
            uchar stegoImagePixel = 0;
            
            tie(secretImagePixel, stegoImagePixel) = checkFEMDCases(pixel1, pixel2, key);

            clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = secretImagePixel;
            clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]) = stegoImagePixel;

            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);

            uchar pixel4 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);

            string binaryPixel4 = byteToBinary(pixel4);
            binaryPixel4[binaryPixel4.size() - 1] = pixel1 % 2;

            uchar clonedImagePixel2 = transformBitsToDecimal(binaryPixel4);
            
            clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]) = clonedImagePixel2;

            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);
        }
    }

    return clonedImages;
}    

// Function to decode a message from a stego image using the LSB matching revisited method
tuple<string, vector<Mat>> decodeOption2(vector<Mat>& images, int length)
{
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

    // // Get the number of message bits to be decoded
    string decodedMessage = "";

    // string decodedHeader = "";

    // while(decodedHeader.size() != 26) {
    //     for(int i = 0; i < images.size() - 1; i += 2) {
    //         // Get the pixel value at the current location
    //         uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
    //         uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);


    //         int message = calculateFEMD(pixel1, pixel2);
    //         string binaryMessage = byteToBinary(message);

    //         decodedHeader += binaryMessage[binaryMessage.size() - 2];
    //         decodedHeader += binaryMessage[binaryMessage.size() - 1];

    //         moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);


    //         uchar pixel4 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);


    //         uchar originalPixel1;
    //         uchar originalPixel2;

    //         int isOriginalImage1PixelEven = pixel4 & 1;


    //         tie(originalPixel1, originalPixel2) = getOriginalPixel(pixel1, pixel2, isOriginalImage1PixelEven, 0);

            
    //         clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = originalPixel1;

    //         moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
    //         moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
    //         moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);
    //     }
    // }

    // cout << decodedHeader << endl;

    // string modeString = decodedHeader.substr(0, 2);
    // string binaryMessage = decodedHeader.substr(2);

    // int mode = transformBitsToDecimal(modeString);
    // int binaryMessageLength = transformBitsToDecimal(binaryMessage);

    // cout << to_string(mode) << endl;
    // cout << to_string(binaryMessageLength) << endl;

    while(decodedMessage.size() != length) {
        for(int i = 0; i < images.size() - 1; i += 2) {
            // Get the pixel value at the current location
            uchar pixel1 = clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]);
            uchar pixel2 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);


            int message = calculateFEMD(pixel1, pixel2);
            string binaryMessage = byteToBinary(message);

            decodedMessage += binaryMessage[binaryMessage.size() - 2];
            decodedMessage += binaryMessage[binaryMessage.size() - 1];

            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);


            uchar pixel4 = clonedImages[i + 1].at<uchar>(imageIndexes[i + 1][0], imageIndexes[i + 1][1]);


            uchar originalPixel1;
            uchar originalPixel2;

            int isOriginalImage1PixelEven = pixel4 & 1;


            tie(originalPixel1, originalPixel2) = getOriginalPixel(pixel1, pixel2, isOriginalImage1PixelEven, 0);

            
            clonedImages[i].at<uchar>(imageIndexes[i][0], imageIndexes[i][1]) = originalPixel1;

            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i], clonedImages[i].rows, clonedImages[i].cols);
            moveIndexForward(imageIndexes[i + 1], clonedImages[i + 1].rows, clonedImages[i + 1].cols);
        }
    }

    // testQuality(originalImage, secretImage);
    return make_tuple(decodedMessage, clonedImages);
}


vector<int> decodeHeader(vector<Mat>& images) {
    string decodedHeader = "";

    vector<int> result;

    while(decodedHeader.size() < 26) {
        for(int i = 0; i < images.size() - 1; i += 2) {
            
        }
    }

    return result;
}

int main()
{
    // Load the cover image and the message
    Mat coverImage1 = imread("../UserImages/mountain.jpg", IMREAD_GRAYSCALE);

    if (coverImage1.empty()) {
        cerr << "Failed to read the cover image." << endl;
        return -1;
    }

    string dbImageFileName = getDBImage(coverImage1.total(), "../DBImages/metadata.txt");

    string dbImageFilePath = "../DBImages/" + dbImageFileName;

    cout << dbImageFilePath << endl;

    Mat coverImage2 = imread(dbImageFilePath, IMREAD_GRAYSCALE);

    if (coverImage2.empty()) {
        cerr << "Failed to read the cover image." << endl;
        return -1;
    }

    // Mat coverImage3 = imread("../images/test_image_3.jpeg", IMREAD_GRAYSCALE);

    // if (coverImage3.empty()) {
    //     cerr << "Failed to read the cover image." << endl;
    //     return -1;
    // }

    vector<Mat> images;
    images.push_back(coverImage1);
    images.push_back(coverImage2);
    // images.push_back(coverImage3);

    bool isBaselineOn = true;

    imshow("Cover Image", coverImage1);
    waitKey(0);

    // Define the secret message to be hidden
    string secretMessage = "This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.This is a secret message. This is going to be a long secret message for testing purpose. Please be patient. Slow is smooth and smooth is fast.";


    if(isBaselineOn) {
        auto start = high_resolution_clock::now();

        vector<Mat> result = encodeLSBMatchingRevisited(coverImage1, secretMessage);

        auto end = high_resolution_clock::now();
        cout << "Encryption time: " << duration_cast<milliseconds>(end - start).count() << "ms" << endl;
        cout << "Capacity: " << coverImage1.total() << endl;

        string binaryMessage = stringToBinary(secretMessage);

        // Display the stego image
        for(int i = 0; i < result.size(); i += 2) {
            Mat secretImage = result[i];
            Mat stegoImage = result[i + 1];

            testQualityBaseline(coverImage1, secretImage, binaryMessage.size());

            imshow("Stego Image " + to_string(i), secretImage);
            waitKey(0);

            imshow("Stego Image " + to_string(i + 1), stegoImage);
            waitKey(0);
        }

        string extractedMessage;
        vector<Mat> restoredImages;

        // Call the function to extract the secret message from the stego image

        auto start1 = high_resolution_clock::now();

        tie(extractedMessage, restoredImages) = decodeLSBMatchingRevisited(result, binaryMessage.size());

        auto end1 = high_resolution_clock::now();
        cout << "Decryption time: " << duration_cast<milliseconds>(end1 - start1).count() << "ms" << endl;


        // Display the extracted secret message
        cout << "Extracted Message: " << binaryToString(extractedMessage) << endl;

        // calculateVarianceMetric(coverImage);

        // Call the function to restore the cover image from the stego image

        for(int i = 0; i < restoredImages.size(); i += 2) {
            imshow("Restored Image " + to_string(i), restoredImages[i]);
            waitKey(0); 
        }
    } else {
        // Call the function to hide the secret message in the cover image
        auto start = high_resolution_clock::now();

        // encodeOption1
        vector<Mat> result = encodeOption2(images, secretMessage);

        auto end = high_resolution_clock::now();
        cout << "Encryption time: " << duration_cast<milliseconds>(end - start).count() << "ms" << endl;
        cout << "Capacity: " << coverImage1.total() << endl;

        string binaryMessage = stringToBinary(secretMessage);

        // Display the stego image
        for(int i = 0; i < result.size(); i += 2) {
            Mat secretImage = result[i];
            Mat stegoImage = result[i + 1];

            // With header
            testQualityOption2(coverImage1, secretImage, binaryMessage.size());

            imshow("Stego Image " + to_string(i), secretImage);
            waitKey(0);

            imshow("Stego Image " + to_string(i + 1), stegoImage);
            waitKey(0);
        }

        // testQuality(coverImage, result.secretImage);
        // testQuality(coverImage, result.stegoImage);

        string extractedMessage;
        vector<Mat> restoredImages;

        // Call the function to extract the secret message from the stego image

        auto start1 = high_resolution_clock::now();

        // decodeOption1
        tie(extractedMessage, restoredImages) = decodeOption2(result, binaryMessage.size());

        auto end1 = high_resolution_clock::now();
        cout << "Decryption time: " << duration_cast<milliseconds>(end1 - start1).count() << "ms" << endl;


        // Display the extracted secret message
        cout << "Extracted Message: " << binaryToString(extractedMessage) << endl;

        // calculateVarianceMetric(coverImage);

        // Call the function to restore the cover image from the stego image

        for(int i = 0; i < restoredImages.size(); i += 2) {
            imshow("Restored Image " + to_string(i), restoredImages[i]);
            waitKey(0); 
        }
        // Mat restoredImage = restoreCoverImage(result.secretImage, result.stegoImage);

        // Display the restored cover image
        // imshow("Restored Image", restoredImage);
        // waitKey(0);

    }

    
    return 0;
}