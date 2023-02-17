#include <iostream>
#include <opencv2/opencv.hpp>

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
        ans += (pow(2, i) * (bits[len - i] == '1' ? 1 : 0));
    }

    return ans;
}

int calculateFEMD(uchar pixel1, uchar pixel2) {
    if(pixel1 > 255 || pixel1 < 0 || pixel2 > 255 || pixel2 < 0) {
        return -1;
    }

    int lsbPixel1 = pixel1 & 1;
    int tempCalc = floor((double)(pixel1)/2) + pixel2;

    int femd1 = lsbPixel1 * 2;
    int femd2 = tempCalc & 1;
    
    return femd1 + femd2;
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
            
            int key = transformBitsToDecimal(messageKey);
            
            if (key == calculateFEMD(pixel, pixel)) {
                secretImage.at<uchar>(row, col) = pixel;
                stegoImage.at<uchar>(row, col) = pixel;
            } else if (key == calculateFEMD(pixel , pixel + 1)) {
                secretImage.at<uchar>(row, col) = pixel;
                stegoImage.at<uchar>(row, col) = pixel + 1;
            } else if (key == calculateFEMD(pixel + 1, pixel - 1)) {
                secretImage.at<uchar>(row, col) = pixel + 1;
                stegoImage.at<uchar>(row, col) = pixel - 1;
            } else if (key == calculateFEMD(pixel + 1, pixel)) {
                secretImage.at<uchar>(row, col) = pixel + 1;
                stegoImage.at<uchar>(row, col) = pixel;
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
    result.stegoImage = secretImage;

    return result;
}

/*
// Function to decode a message from a stego image using the LSB matching revisited method
string decodeLSBMatchingRevisited(Mat &secretImage, Mat &stegoImage)
{
    // Get the number of message bits to be decoded
    int numMessageBits = stegoImage.rows * stegoImage.cols * 3;

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

int main()
{
    // Load the cover image and the message
    Mat coverImage = imread("images/test_image_1.jpeg", IMREAD_GRAYSCALE);

    if (coverImage.empty()) {
        cerr << "Failed to read the cover image." << endl;
        return -1;
    }

    // Define the secret message to be hidden
    string secretMessage = "This is a secret message.";

    // Call the function to hide the secret message in the cover image
    EncodedImages result = encodeLSBMatchingRevisited(coverImage, secretMessage);

    // Display the stego image
    imshow("Secret Image", result.secretImage);
    imshow("Stego Image", result.stegoImage);

    waitKey(0);

    // Call the function to extract the secret message from the stego image
    // string extractedMessage = decodeLSBMatchingRevisited(result.secretImage, result.stegoImage);

    // Display the extracted secret message
    // cout << "Extracted Message: " << extractedMessage << endl;

    // Call the function to restore the cover image from the stego image
    // Mat restoredImage = restoreCoverImage(result.secretImage, result.stegoImage);

    // Display the restored cover image
    // imshow("Restored Image", restoredImage);
    // waitKey(0);

    return 0;
}