#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// Function to generate a secret image from a cover image
Mat generateSecretImage(Mat &coverImage, double targetSSIM)
{
    // TODO: Implement the algorithm to generate a secret image with minimal distortion
    // using the cover image and the target SSIM value

    // Dummy implementation that just returns the cover image
    return coverImage.clone();
}

// Function to encode a message into a cover image using the LSB matching revisited method
void encodeLSBMatchingRevisited(Mat &coverImage, string message)
{
    // Generate a secret image from the cover image with a target SSIM value
    double targetSSIM = 0.95;
    Mat secretImage = generateSecretImage(coverImage, targetSSIM);

    // Get the number of message bits to be encoded
    int numMessageBits = message.size() * 8;

    // Iterate over the pixels of the cover image and the secret image
    int pixelIndex = 0;
    for (int row = 0; row < coverImage.rows; row++)
    {
        for (int col = 0; col < coverImage.cols; col++)
        {
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
        }
    }
}

// Function to decode a message from a stego image using the LSB matching revisited method
string decodeLSBMatchingRevisited(Mat &stegoImage)
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

int main()
{
    // Load the cover image and the message
    Mat coverImage = imread("cover.png", IMREAD_COLOR);
    string message = "Hello, world!";

    // Encode the message into the cover image using the LSB matching revisited method
    encodeLSBMatchingRevisited(coverImage, message);

    // TODO: Save the stego image

    // Load the stego image and decode the message using the LSB matching revisited method
    Mat stegoImage = generateSecretImage(coverImage, 0.95);
    string decodedMessage = decodeLSBMatchingRevisited(stegoImage);
    cout << "Decoded message: " << decodedMessage << endl;

    return 0;
}