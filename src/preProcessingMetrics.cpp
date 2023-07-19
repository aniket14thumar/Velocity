#include <iostream>
#include <chrono>
#include <cmath>
#include <utility>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace std::chrono;
using namespace cv;

float huber_loss(float x, float delta, float sigma) {
    float result;
    float abs_x = abs(x[i][j]);
    float quadratic = 0.5 * pow(abs_x, 2);
    float linear = (delta * abs_x) - (0.5 * pow(delta, 2));
    result = ((abs_x <= delta) ? linear : quadratic) / sigma;
    return result;
}

float dynamic_weighted_average(vector<vector<float>> matrix) {
    int n = matrix.size();
    int m = matrix[0].size();
    vector<vector<float>> weights(n, vector<float>(m, 1.0));
    
    float mean = 0.0;
    float max_val = 0.0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            mean += matrix[i][j];
            max_val = max(max_val, matrix[i][j]);
        }
    }

    mean /= (n * m);
    
    float threshold = 0.1 * max_val;
    float sigma = 0.1 * max_val;
    float delta = 0.4 * max_val;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            float diff = matrix[i][j] - mean;
            if (abs(diff) > threshold) {
                float loss = huber_loss(diff, delta, sigma);
                weights[i][j] = log10(loss);
            }
        }
    }

    float sum_weighted_matrix = 0.0;
    float sum_weights = 0.0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            sum_weighted_matrix += weights[i][j] * matrix[i][j];
            sum_weights += weights[i][j];
        }
    }

    return sum_weighted_matrix / sum_weights;
}

void iterateDynamicAvg(vector<vector<float>> matrix) {
    vector<vector<float>> resultMatrix;
    int rows = matrix.size();
    int cols = matrix[0].size();

    int blockSize = 3;

    for(int i = 0; i < rows; i += 3) {
        vector<float> calculatedAvgRow;

        for(int j = 0; j < cols; j += 3) {
            vector<vector<int>> block;

            // copy the values of the current block into a new block
            for (int k = 0; k < block_size; k++) {
                vector<int> blockRow;

                for (int l = 0; l < block_size; l++) {
                    int row = i + k;
                    int col = j + l;
                    if (row < rows && col < cols) {
                        blockRow.push_back(matrix[row][col]);
                    }
                }

                block.push_back(blockRow);
            }

            float result = dynamic_weighted_average(block);
            calculatedAvgRow.push_back(result);
        }

        resultMatrix.push_back(calculatedAvgRow);
    }
}

double gaussian_weighted_average(vector<vector<double>> matrix) {
    vector<vector<double>> weights(matrix.size(), vector<double>(matrix[0].size(), 1.0));
    double mean = 0.0;
    double max_val = 0.0;
    double threshold = 0.0;
    double sigma = 0.0;
    // calculate mean and max_val
    for (int i = 0; i < matrix.size(); ++i) {
        for (int j = 0; j < matrix[0].size(); ++j) {
            mean += matrix[i][j];
            max_val = max(max_val, matrix[i][j]);
        }
    }

    mean /= (matrix.size() * matrix[0].size());
    threshold = 0.4 * max_val;
    sigma = 0.2 * max_val;

    // calculate weights
    for (int i = 0; i < matrix.size(); ++i) {
        for (int j = 0; j < matrix[0].size(); ++j) {
            if (abs(matrix[i][j] - mean) > threshold) {
                weights[i][j] = exp(-pow((matrix[i][j] - mean) / sigma, 2));
            }
        }
    }

    // calculate weighted_average
    double sum_weights = 0.0;
    double sum_weighted_matrix = 0.0;
    for (int i = 0; i < matrix.size(); ++i) {
        for (int j = 0; j < matrix[0].size(); ++j) {
            sum_weights += weights[i][j];
            sum_weighted_matrix += weights[i][j] * matrix[i][j];
        }
    }

    double weighted_average = sum_weighted_matrix / sum_weights;
    return weighted_average;
}

void iterateGaussianAvg(vector<vector<float>> matrix) {
    vector<vector<float>> resultMatrix;
    int rows = matrix.size();
    int cols = matrix[0].size();

    int blockSize = 3;

    for(int i = 0; i < rows; i += 3) {
        vector<float> calculatedAvgRow;

        for(int j = 0; j < cols; j += 3) {
            vector<vector<int>> block;

            // copy the values of the current block into a new block
            for (int k = 0; k < block_size; k++) {
                vector<int> blockRow;

                for (int l = 0; l < block_size; l++) {
                    int row = i + k;
                    int col = j + l;
                    if (row < rows && col < cols) {
                        blockRow.push_back(matrix[row][col]);
                    }
                }

                block.push_back(blockRow);
            }

            float result = gaussian_weighted_average(block);
            calculatedAvgRow.push_back(result);
        }

        resultMatrix.push_back(calculatedAvgRow);
    }
}

void calculateVarianceMetric(Mat &image) {
    vector<vector<float>> result;

    result.resize(image.rows);

    int n = image.cols;

    for (int row = 0; row < image.rows; row++)
    {
        float sum = 0.0;

        for(int col = 0; col < image.cols; col++) {
            sum += image.at<uchar>(row, col);
        }

        float mean = sum / n;

        float variance = 0.0;

        for(int col = 0; col < image.cols; col++) {
            variance += pow(image.at<uchar>(row, col) - mean, 2);
        }

        variance /= n;

        result[row].push_back(mean);
        result[row].push_back(sqrt(variance));

        for(int i = 0; i < 2; i++) {
            cout << result[row][i] << " ";
        }

        cout << endl;

    }
}

int calcWeightedAvg(Mat& input_matrix, int row, int col) {
    float sum = 0, count = 0;
    
    for (int i = row; i < row + 3; i++) {
        for (int j = col; j < col + 3; j++) {
            if (i >= 0 && i < input_matrix.rows && j >= 0 && j < input_matrix.cols) {
                int pixel = input_matrix.at<uchar>(i, j);
                if((pixel >= 0 && pixel <= 50) || (pixel <= 250 && pixel >= 200)) {
                    sum += 2 * pixel;
                    count += 2;
                } else if((pixel >= 51 && pixel <= 75) || (pixel <= 199 && pixel >= 175)) {
                    sum += (1.5) * pixel;
                    count += 1.5;
                } else {
                    sum += pixel;
                    count++;
                }
            }
        }
    }
    
    return count == 0 ? 0 : sum / count;
}

int calcAvg(Mat& input_matrix, int row, int col) {
    int sum = 0, count = 0;
    for (int i = row; i < row + 3; i++) {
        for (int j = col; j < col + 3; j++) {
            if (i >= 0 && i < input_matrix.rows && j >= 0 && j < input_matrix.cols) {
                sum += input_matrix.at<uchar>(i, j);
                count++;
            }
        }
    }
    return count == 0 ? 0 : sum / count;
}

// Calculate the average of each 3x3 block in the input matrix and store it in the output matrix
void calcAvgMatrix(Mat& input_matrix) {
    int rows = input_matrix.rows;
    int cols = input_matrix.cols;
    vector<vector<int>> aux_matrix(rows, vector<int>(cols, 0));
    vector<vector<int>> output_matrix;
    int row = 0;
    int col = 0;
    
    for (int i = 0; i < rows; i++) {
        if(i % 3 == 0) {
            row++;
        }
        for (int j = 0; j < cols; j++) {
            if(col % 3 == 0) {
                col++;
            }
            if (aux_matrix[i][j] == 0) { // if the 3x3 block has not been calculated yet
                int avg = calc_avg(input_matrix, i, j); // calculate the average of the 3x3 block
                output_matrix[row][col] = avg; // store the average in the output matrix
                aux_matrix[i][j] = aux_matrix[i][j+1] = aux_matrix[i][j+2] = 
                    aux_matrix[i+1][j] = aux_matrix[i+1][j+1] = aux_matrix[i+1][j+2] =
                    aux_matrix[i+2][j] = aux_matrix[i+2][j+1] = aux_matrix[i+2][j+2] = 1; // mark the 3x3 block as calculated in the auxiliary matrix
            }
        }
    }
}