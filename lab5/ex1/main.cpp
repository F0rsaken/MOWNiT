#include <cstdlib>
#include <iostream>
#include <time.h>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <tgmath.h>
#include "../../lib/matrix-lib.h"
#include "../../lib/interpolation-lib.h"
using namespace std;

double step = 0.1;

double a = -3 * M_PI;
double b = 2 * M_PI;

// y = e^(-3( sin(x) ))
double fX(double x) {
    return exp( -3 * sin(x) );
}

void drawOriginalPlot() {
    int n = ((b - a) / step) + 1;
    Point *dataPoints = new Point[n];
    double x = a;
    for (int i = 0; i < n-1; i++, x += step) {
        dataPoints[i].x = x;
        dataPoints[i].y = fX(x);
    }
    dataPoints[n-1].x = b;
    dataPoints[n-1].y = fX(b);
    sendPlotToFile(dataPoints, n, "original_plot.dat", true);
}

Point* getDataPoints(int n) {
    double singleStep = (b-a)/(n-1);
    Point *data = new Point[n];
    data[0].x = a;
    data[n-1].x = b;
    // cout << "a, b, step: " << data[0].x << "; " << data[n-1].x << "; " << singleStep << "\n\n";
    for (int i = 1; i < n-1; i++) {
        data[i].x = data[i-1].x + singleStep;
    }

    for (int i = 0; i < n; i++) {
        data[i].y = fX(data[i].x);
        // cout << "x[" << i << "]: " << data[i].x << ",  " << "y[" << i << "]: " << data[i].y << endl;
    }

    return data;
}

/**
 * Argumenty:
 *  - warunek brzegowy: 0 | 1 ( clamped | natural )
 *  - ilosc przedziałow: n
 */

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        cout << "Zła liczba argumentów!\n";
        return 1;
    }

    int boundaryType = atoi(argv[1]); // 0 - clamped, 1 - natural
    int n = atoi(argv[2]);
    
    if (boundaryType != 0 && boundaryType != 1) {
        cout << "Zła wartość pierwszego argumentu!\n";
        return 1;
    }

    Point *data = getDataPoints(n);
    CubicSpline *splines = cubicSplines(data, n, boundaryType);

    cout << "original x: " << data[0].x << " y: " << data[0].y << endl;
    cout << "counted  x: " << data[0].x << " y: " << splines[0].fx(data[0].x) << endl;

    for (int i = 1; i < n; i++) {
        cout << "original x: " << data[i].x << " y: " << data[i].y << endl;
        cout << "counted  x: " << data[i].x << " y: " << splines[i - 1].fx(data[i].x) << endl;
    }
    
    int outN = ((data[n - 1].x - data[0].x) / step) + 1;
    // cout << "OutN: " << outN << endl;
    Point *interpolation = new Point[outN];
    double xi = data[0].x;

    for (int i = 0; i < outN; xi += step, i++) {
        interpolation[i].x = xi;

        for (int j = 1; j < n; j++) {
            if ( (xi <= splines[j-1].x2) && (xi >= splines[j-1].x1) ) {
                interpolation[i].y = splines[j-1].fx(xi);
                break;
            }
        }
    }
    
    cout << "Interpolacja: \n";
    sendPlotToFile(interpolation, outN, "out.dat", true);

    drawOriginalPlot();

    delete[] data;
    delete[] splines;

    return 0;
}
