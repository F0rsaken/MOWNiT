#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#define _USE_MATH_DEFINES
#include <tgmath.h>
#include "interpolation-lib.h"
#include "matrix-lib.h"
using namespace std;

Polynomial::Polynomial(double a, double b, double c, double d) {
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
}

double Polynomial::f(double x) {
    double retVal = ( this->a * pow(x, 3) ) + (this->b * pow(x, 2)) + (this->c * x) + this->d;
    return retVal;
}

PolynomialFunction::PolynomialFunction() {}

PolynomialFunction::PolynomialFunction(int size) {
    this->factors = new double[size];
    this->size = size;
}

PolynomialFunction::PolynomialFunction(int size, double *points) {
    this->size = size;
    this->factors = points;
}

PolynomialFunction::~PolynomialFunction() {
    delete[] this->factors;
}

double QuadraticSpline::fx(double x) {
    double tmp = (( (this->D2*pow(x - this->x1, 2) ) - (this->D1*pow(this->x2-x, 2)) )/(2*this->h));
    return tmp + this->y - ( (this->D2 * this->h)/2 );
}

double CubicSpline::fx(double x) {
    double tmp1 = ((this->M1 * pow(this->x2 - x, 3)) + (this->M2 * pow(x - this->x1, 3)))/(6*this->h);
    double tmp2 = ((this->y1/this->h) - ((this->M1 * this->h)/6)) * (this->x2 - x);
    double tmp3 = ((this->y2/this->h) - ((this->M2 * this->h)/6)) * (x - this->x1);
    return tmp1 + tmp2 + tmp3;
}



double PolynomialFunction::f(double x) {
    if (this->size <= 0) {
        return 0;
    }

    double retVal = this->factors[0];
    for (int i = 1; i < this->size; i++) {
        retVal += (this->factors[i] * pow(x, i));
    }

    return retVal;
}

void sendPlotToFile(Point data[], int n, string fileName, bool informUser) {
    // cout << "Interpolacja: \n";
    ofstream outputFile;
    outputFile.open(fileName, ios::trunc);
    for(int i = 0; i < n; i++) {
        outputFile << data[i].x << " " << data[i].y << endl;
    }
    outputFile.close();
    if (informUser) cout << "Skończono pisać do pliku " << fileName << endl;
}

double useLagrange(Point data[], int n, double xi) {
    double retVal = 0, upper, lower, li;
    for (int i = 0; i < n; i++) {

        upper = 1, lower = 1;
        for (int j = 0; j < n; j++) {
            if (j != i) {
                upper = upper * (xi - data[j].x);
                lower = lower * (data[i].x - data[j].x);
            }
        }
        li = upper/lower;
        retVal += (data[i].y * li);
    }
    return retVal;
}

double useNewton(Point data[], int n, double x, double *diffTable[]) {
    double retVal = diffTable[0][0];
    double ni;

    for (int i = 1; i < n; i++) {
        ni = 1;
        for (int j = 0; j < i; j++) {
            ni = ni * (x-data[j].x);
        }
        retVal += diffTable[i][i] * ni;
    }

    return retVal;
}

void fillNewtonDiffTable(int n, Point data[], double *diffTable[]) {
    for (int i = 0; i < n; i++) { diffTable[i][0] = data[i].y; }

    double upper;
    double lower;

    for (int i = 1; i < n; i++) {
        // i-ta kolumna, j-ty wiersz
        for (int j = i; j < n; j++) {
            upper = diffTable[j][i - 1] - diffTable[j - 1][i - 1];
            lower = data[j].x - data[j-i].x;
            diffTable[j][i] = upper/lower;
        }
    }
}

double useHermit(Point data[], int n, double x, double *diffTable[]) {
    double retVal = diffTable[0][1];
    double ni;
    int rows = 2 * n;
    int columns = (2 * n) + 1;

    // naprawic ten moment
    for (int i = 2; i < columns; i++) {
        ni = 1;
        for (int j = 0; j < i-1; j++) {
            ni = ni * (x - diffTable[j][0]);
        }
        retVal += diffTable[i-1][i] * ni;
    }

    return retVal;
}

void fillHermitDiffTable(int n, Point data[], double *diffTable[], double derivatives[]) {
    int rows = 2*n;
    int columns = (2*n)+1;
    for (int i = 0; i < n; i++) {
        diffTable[2*i][0] = data[i].x;
        diffTable[(2*i)+1][0] = data[i].x;
        diffTable[2*i][1] = data[i].y;
        diffTable[(2*i)+1][1] = data[i].y;
    }

    diffTable[1][2] = derivatives[0];

    int index;
    for (int i = 1; i < n; i++) {
        index = 2*i;
        diffTable[index][2] = (diffTable[index][1] - diffTable[index-1][1]) / (diffTable[index][0] - diffTable[index-1][0]);
        diffTable[index+1][2] = derivatives[i];
    }

    double upper;
    double lower;

    for (int i = 3; i < columns; i++) {
        // i-ta kolumna, j-ty wiersz
        for (int j = i-1; j < rows; j++) {
            upper = diffTable[j][i - 1] - diffTable[j - 1][i - 1];
            lower = diffTable[j][0] - diffTable[j-i+1][0];
            diffTable[j][i] = upper/lower;
        }
    }
}


CubicSpline * cubicSplines(Point *data, int n, int boundaryType) {
    // hi = xi - xi-1
    double *h = new double[n-1];
    for (int i = 0; i < n-1; i++) { h[i] = data[i+1].x - data[i].x; }

    double *lower = new double[n-1]; // mi
    double *upper = new double[n-1]; // lambda
    double tmp;
    for (int i = 0; i < n-2; i++) {
        tmp = h[i] + h[i+1];
        lower[i] = h[i]/tmp;
        upper[i+1] = h[i+1]/tmp;
    }

    // FIXME: dyferencjal
    double *diff = new double[n]; // f[xi-1, xi, xi+1]
    double *midVector = new double[n];
    for (int i = 0; i < n; i++) { midVector[i] = 2; }

    if (boundaryType == 1) { // natural boundary condition
        midVector[0] = 1; midVector[n-1] = 1;
        lower[n-2] = 0;
        upper[0] = 0;
        diff[0] = 0; diff[n-1] = 0;
    } else { // clamped boundary condition
        upper[0] = 1;
        lower[n-2] = 1;
        diff[0] = (6*( (data[1].y - data[0].y) / (data[1].x - data[0].x) ))/(data[1].x - data[0].x);
        diff[n-1] = (6*( (data[n-1].y - data[n-2].y) / data[n-1].x - data[n-2].x ))/(data[n-1].x - data[n-2].x);
        // v[0] = 6*( (data[1].y - data[0].y)/(h[0]*( data[1].x - data[0].x ) ) );
        // v[n-1] = 6*( (data[n-1].y - data[n-2].y)/(h[n-2]*( data[n-1].x - data[n-2].x ) ) );
    }

    for (int i = 1; i < n-1; i++) {
        diff[i] = (6*(((data[i+1].y - data[i].y)/(data[i+1].x - data[i].x))
                - ((data[i].y - data[i-1].y)/(data[i].x - data[i-1].x))))
            /(data[i+1].x - data[i-1].x);
    }


    double *M = thomasAlgorithm(lower, midVector, upper, diff, n);
    CubicSpline *retSplines = new CubicSpline[n-1];

    for (int i = 0; i < n-1; i++) {
        retSplines[i].x1 = data[i].x;
        retSplines[i].x2 = data[i+1].x;
        retSplines[i].y1 = data[i].y;
        retSplines[i].y2 = data[i+1].y;
        retSplines[i].M1 = M[i];
        retSplines[i].M2 = M[i+1];
        retSplines[i].h = h[i];
    }

    delete[] h;
    delete[] lower;
    delete[] upper;
    delete[] midVector;
    delete[] diff;
    delete[] M;

    return retSplines;
}

QuadraticSpline * quadraticSplines(Point *data, int n, double D0) {
    double *h = new double[n-1];
    for (int i = 0; i < n-1; i++) { h[i] = data[i+1].x - data[i].x; }

    double *D = new double[n];
    D[0] = D0;
    for (int i = 1; i < n; i++) {
        D[i] = ( ( (2*data[i].y) - (2*data[i-1].y) )/h[i-1] ) - D[i-1];
    }

    QuadraticSpline *retSplines = new QuadraticSpline[n-1];

    for (int i = 0; i < n-1; i++) {
        retSplines[i].x1 = data[i].x;
        retSplines[i].x2 = data[i+1].x;
        retSplines[i].h = h[i];
        retSplines[i].y = data[i+1].y;
        retSplines[i].D1 = D[i];
        retSplines[i].D2 = D[i+1];
    }

    delete[] h;
    delete[] D;

    return retSplines;
}

PolynomialFunction polynomialRegression(Point *data, int n, int m) {
    // dla m+1 = n - Interpolacja
    // dla m+1 <= n-1 (m <= n-2) - aproksymacja
    if (m > n-2) {
        cout << "m jest za duze, zostanie zmniejszone do minimum" << endl;
        m = n-2;
    }

    int nSize = n+1, mSize = m+1;

    double **matrix = new double*[mSize];
    for (int i = 0; i < mSize; i++) {
        matrix[i] = new double[mSize];
    }

    // tablica sum po x od 0 do 2m, dla skrocenia obliczen
    int xSumsSize = (2*m)+1;
    double *xSums = new double[xSumsSize];
    xSums[0] = 1;
    for (int i = 1; i < xSumsSize; i++) {
        xSums[i] = 0;
        for (int j = 0; j < nSize; j++) {
            xSums[i] += pow(data[j].x, i);
        }
    }

    // wypelniamy macierz wspolczynnikow
    for (int i = 0; i < mSize; i++) {
        for (int j = 0; j < mSize; j++) {
            matrix[i][j] = xSums[i+j];
        }
    }
    matrix[0][0] = nSize;

    // wypelniamy rozwiazania, czyli sumy y*x^i
    double *freeVector = new double[mSize];
    for (int i = 0; i < mSize; i++) {
        freeVector[i] = 0;
        for (int j = 0; j < nSize; j++) {
            freeVector[i] += data[j].y * pow(data[j].x, i);
        }
    }

    // double *initVector = new double[n];
    // for (int i = 0; i < n; i++) { initVector[i] = 1; }
    // double *xVector = SORAlgorithm(m, (const double**)matrixinitVector, freeVector, 0.1, 1, 1);
    double *xVector = gaussElimination(mSize, mSize, matrix, freeVector);
    // double *xVector = jacobiAlgorithm(m, (const double**)matrix, initVector, freeVector, 1, 1);

    PolynomialFunction aproximation = PolynomialFunction(mSize, xVector);

    for (int i = 0; i < mSize; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;
    delete[] xSums;
    delete[] freeVector;

    return aproximation;
}

double ** countFactorsForTrygonometricApproximation(Point *dataPoints, int nPoints, int degree, double rangeM) {
    // dataPoints musi byc przesuniete o 0.5pi
    // range jest bez PI
    if (nPoints%2 == 0) { // parzysta ilosc punktow
        if (degree > ((nPoints-2)/2)) {
            degree = (nPoints-2)/2;
            cout << "Stopien wielomianu został obniony do minimalnej wartości " << degree << endl;
        }
    } else { // nieparzysta ilosc punktow
        if ( degree > ( (nPoints-1)/2 ) ) {
            degree = (nPoints-1)/2;
            cout << "Stopien wielomianu został obniony do minimalnej wartości " << degree << endl;
        }
    }

    double **factorsTable = new double*[2];
    int aSize = degree+2;
    int bSize = degree;
    factorsTable[0] = new double[aSize];
    factorsTable[1] = new double[bSize+1];

    // w pierwszej komorce wspolczynnikow b jest przechowywany stopien wielomianu
    factorsTable[1][0] = degree;

    // dodac k * pi/I * xi

    // wyliczanie a0
    factorsTable[0][0] = 0;
    for (int j = 0; j < nPoints; j++) {
        factorsTable[0][0] += ( dataPoints[j].y );
    }
    factorsTable[0][0] *= 2;
    factorsTable[0][0] /= nPoints;

    for (int i = 1; i <= bSize; i++) {
        // coś * 2 i potem przez n
        factorsTable[0][i] = 0; factorsTable[1][i] = 0;
        for (int j = 0; j < nPoints; j++) {
            // punkt a
            factorsTable[0][i] += ( dataPoints[j].y * cos( (i*dataPoints[j].x)/rangeM ) );
            // punkt b
            factorsTable[1][i] += ( dataPoints[j].y * sin( (i*dataPoints[j].x)/rangeM ) );
        }
        factorsTable[0][i] *= 2; factorsTable[1][i] *= 2;
        factorsTable[0][i] /= nPoints; factorsTable[1][i] /= nPoints;
    }

    // wyliczamy a(m+1)
    int index = aSize-1;
    factorsTable[0][index] = 0;
    for (int j = 0; j < nPoints; j++) {
        factorsTable[0][index] += ( dataPoints[j].y * cos((index * dataPoints[j].x)/rangeM) );
    }
    factorsTable[0][index] *= 2;
    factorsTable[0][index] /= nPoints;

    return factorsTable;
}

double trygonometricApproximation(double *factorsTable[], int nPoints, double rangeM, double x) {
    double *aFactors = factorsTable[0];
    double *bFactors = factorsTable[1];
    int degree = bFactors[0];

    double outVal = aFactors[0]/2;

    double aSum = 0;
    double bSum = 0;
    double xTmp;
    for (int i = 1; i <= degree; i++) {
        xTmp = (i * x)/rangeM;
        aSum += (aFactors[i] * cos(xTmp));
        bSum += (bFactors[i] * sin(xTmp));
    }

    if (nPoints%2 == 0) {
        aSum += (aFactors[degree+1] * cos( ((degree+1) * x)/rangeM ))/2;
    }

    outVal += aSum + bSum;

    return outVal;
}