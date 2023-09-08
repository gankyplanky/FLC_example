#include <iostream>
#include <stdlib.h>
#include <windows.h>
#include <string>
#include <thread>
#include <fstream>
#include <cstring>
#include <vector>
#include <math.h>
#include <conio.h>
#pragma warning(disable : 4996)

//Predefinisane granice za funkcije pripadnosti pobeda kluba
#define sigmfCurvePobedeMalo 1.5
#define sigmfMidPobedeMalo 6

#define trapmfStartPobedeProsek 5
#define trapmfBeginFlatPobedeProsek 10
#define trapmfStopFlatPobedeProsek 15
#define trapmfEndPobedeProsek 20

#define sigmfCurvePobedePuno 1.7
#define sigmfMidPobedePuno 17

//Predefinisane granice za funkcije pripadnosti datih golova kluba
#define sigmfCurveGoloviMalo 0.8
#define sigmfMidGoloviMalo 12

#define trapmfStartGoloviProsek 10
#define trapmfBeginFlatGoloviProsek 20
#define trapmfStopFlatGoloviProsek 30
#define trapmfEndGoloviProsek 40

#define sigmfCurveGoloviPuno 0.8
#define sigmfMidGoloviPuno 38

//Predefinisane granice za funkcije pripadnosti proseka datih golova po mecu kluba
#define sigmfCurveAvgGoloviMalo 1
#define sigmfMidAvgGoloviMalo 0.5

#define sigmfCurveAvgGoloviPuno 1
#define sigmfMidAvgGoloviPuno 1.5

//Predefinisane granice za funkcije pripadnosti izlaza
#define sigmfCurveTimLos 3
#define sigmfMidTimLos 3

#define gbellCenterTimProsecan 5
#define gbellWidthTimProsecan 0.7

#define sigmfCurveTimDobar 3
#define sigmfMidTimDobar 7

//Kontrolise broj decimala brojeva u outputSpace.space i alpha odsecanja
#define outputSpaceDecimalPrecision 0.000001

using namespace std;

void runWebScraper() 
{
    system("python webScraper.py");
}

class Klub 
{
    public:
        int pobede;
        int golovi;
        double prosekGolovi;
        string naziv;

        Klub(int pobede, int golovi, double prosekGolovi, string naziv) 
        {
            this->golovi = golovi;
            this->pobede = pobede;
            this->prosekGolovi = prosekGolovi;
            this->naziv = naziv;
        }

        Klub(const Klub& parent)
        {
            this->pobede = parent.pobede;
            this->golovi = parent.golovi;
            this->prosekGolovi = parent.prosekGolovi;
            this->naziv = parent.naziv;
        }

        Klub()
        {
            pobede = 0;
            golovi = 0;
            prosekGolovi = 0;
        }

        bool hasData()
        {
            if (pobede == 0 && golovi == 0 && prosekGolovi == 0)
                return false;
            return true;
        }
};

class membershipFunc 
{
    protected:
        vector<double> parameters;

    public:
        membershipFunc() {}

        virtual double calculate(double x)
        {
            return x;
        }

        void setParams() {}

        vector<double> getParams()
        {
            return parameters;
        }
};

class trapMF : public membershipFunc
{
    public:
        trapMF(double start, double flatStart, double flatEnd, double end)
        {
            parameters.clear();
            parameters.push_back(start);
            parameters.push_back(flatStart);
            parameters.push_back(flatEnd);
            parameters.push_back(end);
        }
        
        void setParams(double start, double flatStart, double flatEnd, double end)
        {
            parameters.clear();
            parameters.push_back(start);
            parameters.push_back(flatStart);
            parameters.push_back(flatEnd);
            parameters.push_back(end);
        }

        double calculate(double x) 
        {
            return max(min(min(((x - parameters[0]) / (parameters[1] - parameters[0])), ((parameters[3] - x) / (parameters[3] - parameters[2]))), 1), 0);
        }
};

class sigMF : public membershipFunc
{
    private:
        bool isReverse = false;

    public:
        sigMF(double curveStrenght, double curveMedian, bool pIsReverse)
        {
            parameters.clear();
            isReverse = pIsReverse;
            parameters.push_back(curveStrenght);
            parameters.push_back(curveMedian);
        }

        void setParams(double curveStrenght, double curveMedian)
        {
            parameters.clear();
            parameters.push_back(curveStrenght);
            parameters.push_back(curveMedian);
        }

        double calculate(double x)
        {
            if(isReverse)
                return 1 / (1 + exp(parameters[0] * (x - parameters[1])));
            return 1 / (1 + exp(-(parameters[0] * (x - parameters[1]))));
        }
};

class gBellMF : public membershipFunc
{
    public:
        gBellMF(double waveCenter, double waveWidth)
        {
            parameters.clear();
            parameters.push_back(waveCenter);
            parameters.push_back(waveWidth);
        }

        void setParams(double waveCenter, double waveWidth)
        {
            parameters.clear();
            parameters.push_back(waveCenter);
            parameters.push_back(waveWidth);
        }

        double calculate(double x)
        {
            return exp((-pow((x-parameters[0]), 2)) / (2 * pow(parameters[1], 2)));
        }
};

class outputSpace 
{
    private:
        vector<vector<double>> space;
        double startRange;
        double stopRange;
        double precision;
        double totalColumns;
        double totalRules;

        double getColumnMax(int column)
        {
            double max = 0;
            for (int i = 0; i < totalRules; i++)
                if (space[i][column] > max)
                    max = space[i][column];
            return max;
        }

    public:
        outputSpace(double pTotalRules, double pStartRange, double pStopRange, double pPrecision) 
        {
            totalRules = pTotalRules;
            startRange = pStartRange;
            stopRange = pStopRange;
            precision = pPrecision;
            totalColumns = std::round((stopRange - startRange) / precision);

            for (int i = 0; i < totalRules + 1; i++) {
                vector<double> tempVector;

                for (int j = 0; j < totalColumns; j++)
                    tempVector.push_back(0);

                space.push_back(tempVector);
            }
                
        }

        void setSpace(int row, membershipFunc& mf) 
        {
            double tempValue = startRange;

            for (int i = 0; i < totalColumns; i++)
            {
                space[row][i] = std::round(mf.calculate(tempValue) / outputSpaceDecimalPrecision) * outputSpaceDecimalPrecision;
                tempValue += precision;
            }
        }

        void alphaCut(int row, double alpha, bool debugMode)
        {      
            alpha = std::round(alpha / outputSpaceDecimalPrecision) * outputSpaceDecimalPrecision;
            if(debugMode)
                cout << "alpha: " << alpha << endl;
            for (int i = 0; i < totalColumns; i++)
                if (space[row][i] > alpha)
                    space[row][i] = alpha;
                   
        }

        double getCOG(bool debugMode)
        {
            //Agregacija izlaza
            for (int i = 0; i < totalColumns; i++)
                space[totalRules][i] = getColumnMax(i);

            double result = 0, maxVal = 0, maxValCount = 1;

            //Najveci
            for (int i = 0; i < totalColumns; i++)
                if (space[totalRules][i] > maxVal)
                    maxVal = space[totalRules][i];

            
            for(int i = 0; i < totalColumns; i++)
                if (space[totalRules][i] == maxVal) {
                    maxValCount++;
                    result += maxVal * i;
                }

            if(debugMode)
                cout << "max: " << maxVal << endl;
            result = result / (maxVal * maxValCount);
            if (debugMode)
                cout << "Centar gravitacije/Ocena tima: " << result << endl;
            return result;
            
        }
};

char* getPercentage(const Klub& klub1, const Klub& klub2, bool debugMode)
{
    //Funkcije pripadnosti za pobede
    sigMF maloPobeda = sigMF(sigmfCurvePobedeMalo, sigmfMidPobedeMalo, true);
    trapMF prosecnoPobeda = trapMF(trapmfStartPobedeProsek, trapmfBeginFlatPobedeProsek, trapmfBeginFlatPobedeProsek, trapmfEndPobedeProsek);
    sigMF punoPobeda = sigMF(sigmfCurvePobedePuno, sigmfMidPobedePuno, false);

    //Funkcije pripadnosti za golove
    sigMF maloGolova = sigMF(sigmfCurveGoloviMalo, sigmfMidGoloviMalo, true);
    trapMF prosecnoGolova = trapMF(trapmfStartGoloviProsek, trapmfBeginFlatGoloviProsek, trapmfBeginFlatGoloviProsek, trapmfEndGoloviProsek);
    sigMF punoGolova = sigMF(sigmfCurveGoloviPuno, sigmfMidGoloviPuno, false);

    //Funkcije pripadnosti za prosecne golove po mecu
    sigMF maloAvgGolova = sigMF(sigmfCurveAvgGoloviMalo, sigmfMidAvgGoloviMalo, true);
    sigMF punoAvgGolova = sigMF(sigmfCurveAvgGoloviPuno, sigmfMidAvgGoloviPuno, false);

    //Izlazne funkcije pripadnosti
    sigMF timIgraLose = sigMF(sigmfCurveTimLos, sigmfMidTimLos, true);
    gBellMF timIgraProsecno = gBellMF(gbellCenterTimProsecan, gbellWidthTimProsecan);
    sigMF timIgraDobro = sigMF(sigmfCurveTimDobar, sigmfMidTimDobar, false);

    //OutputSpace-evi za timove
    outputSpace tim1Space = outputSpace(3, 0, 10, 0.1);
    outputSpace tim2Space = outputSpace(3, 0, 10, 0.1);

    //'Then' deo pravila
    tim1Space.setSpace(0, timIgraLose);
    tim1Space.setSpace(1, timIgraProsecno);
    tim1Space.setSpace(2, timIgraDobro);

    tim2Space.setSpace(0, timIgraLose);
    tim2Space.setSpace(1, timIgraProsecno);
    tim2Space.setSpace(2, timIgraDobro);
    
    //Pravila
    if (debugMode)
        cout << "\nAlpha odsecanja tima 1:\n";
    tim1Space.alphaCut(0, max(
        min(maloPobeda.calculate(klub1.pobede), min(maloGolova.calculate(klub1.golovi), maloAvgGolova.calculate(klub1.prosekGolovi))),
        max(
            min(prosecnoPobeda.calculate(klub1.pobede), min(maloGolova.calculate(klub1.golovi), maloAvgGolova.calculate(klub1.prosekGolovi))),
            max(
                min(maloPobeda.calculate(klub1.pobede), min(prosecnoGolova.calculate(klub1.golovi), maloAvgGolova.calculate(klub1.prosekGolovi))), 
                min(maloPobeda.calculate(klub1.pobede), min(maloGolova.calculate(klub1.golovi), punoAvgGolova.calculate(klub1.prosekGolovi)))
            )
        )
    ), debugMode);
    tim1Space.alphaCut(1, max(
        min(prosecnoPobeda.calculate(klub1.pobede), min(prosecnoGolova.calculate(klub1.golovi), maloAvgGolova.calculate(klub1.prosekGolovi))),
        max(
            min(prosecnoPobeda.calculate(klub1.pobede), min(maloGolova.calculate(klub1.golovi), punoAvgGolova.calculate(klub1.prosekGolovi))),
            max(
                min(maloPobeda.calculate(klub1.pobede), min(prosecnoGolova.calculate(klub1.golovi), punoAvgGolova.calculate(klub1.prosekGolovi))),
                max(
                    min(prosecnoPobeda.calculate(klub1.pobede), min(prosecnoGolova.calculate(klub1.golovi), punoAvgGolova.calculate(klub1.prosekGolovi))),
                    max(
                        min(prosecnoPobeda.calculate(klub1.pobede), min(punoGolova.calculate(klub1.golovi), maloAvgGolova.calculate(klub1.prosekGolovi))),
                        min(punoPobeda.calculate(klub1.pobede), min(prosecnoGolova.calculate(klub1.golovi), maloAvgGolova.calculate(klub1.prosekGolovi)))
                    )
                )
            )
        )
    ), debugMode);
    tim1Space.alphaCut(2, max(
        min(maloPobeda.calculate(klub1.pobede), min(punoGolova.calculate(klub1.golovi), maloAvgGolova.calculate(klub1.prosekGolovi))),
        max(
            min(punoPobeda.calculate(klub1.pobede), min(prosecnoGolova.calculate(klub1.golovi), punoAvgGolova.calculate(klub1.prosekGolovi))),
            max(
                min(prosecnoPobeda.calculate(klub1.pobede), min(punoGolova.calculate(klub1.golovi), punoAvgGolova.calculate(klub1.prosekGolovi))),
                min(punoPobeda.calculate(klub1.pobede), min(punoGolova.calculate(klub1.golovi), punoAvgGolova.calculate(klub1.prosekGolovi)))
            )
        )
    ), debugMode);

    if (debugMode)
        cout << "\nAlpha odsecanja tima 2:\n";
    tim2Space.alphaCut(0, max(
        min(maloPobeda.calculate(klub2.pobede), min(maloGolova.calculate(klub2.golovi), maloAvgGolova.calculate(klub2.prosekGolovi))),
        max(
            min(prosecnoPobeda.calculate(klub2.pobede), min(maloGolova.calculate(klub2.golovi), maloAvgGolova.calculate(klub2.prosekGolovi))),
            max(
                min(maloPobeda.calculate(klub2.pobede), min(prosecnoGolova.calculate(klub2.golovi), maloAvgGolova.calculate(klub2.prosekGolovi))),
                min(maloPobeda.calculate(klub2.pobede), min(maloGolova.calculate(klub2.golovi), punoAvgGolova.calculate(klub2.prosekGolovi)))
            )
        )
    ), debugMode);
    tim2Space.alphaCut(1, max(
        min(prosecnoPobeda.calculate(klub2.pobede), min(prosecnoGolova.calculate(klub2.golovi), maloAvgGolova.calculate(klub2.prosekGolovi))),
        max(
            min(prosecnoPobeda.calculate(klub2.pobede), min(maloGolova.calculate(klub2.golovi), punoAvgGolova.calculate(klub2.prosekGolovi))),
            max(
                min(maloPobeda.calculate(klub2.pobede), min(prosecnoGolova.calculate(klub2.golovi), punoAvgGolova.calculate(klub2.prosekGolovi))),
                max(
                    min(prosecnoPobeda.calculate(klub2.pobede), min(prosecnoGolova.calculate(klub2.golovi), punoAvgGolova.calculate(klub2.prosekGolovi))),
                    max(
                        min(prosecnoPobeda.calculate(klub2.pobede), min(punoGolova.calculate(klub2.golovi), maloAvgGolova.calculate(klub2.prosekGolovi))),
                        min(punoPobeda.calculate(klub2.pobede), min(prosecnoGolova.calculate(klub2.golovi), maloAvgGolova.calculate(klub2.prosekGolovi)))
                    )
                )
            )
        )
    ), debugMode);
    tim2Space.alphaCut(2, max(
        min(maloPobeda.calculate(klub2.pobede), min(punoGolova.calculate(klub2.golovi), maloAvgGolova.calculate(klub2.prosekGolovi))),
        max(
            min(punoPobeda.calculate(klub2.pobede), min(prosecnoGolova.calculate(klub2.golovi), punoAvgGolova.calculate(klub2.prosekGolovi))),
            max(
                min(prosecnoPobeda.calculate(klub2.pobede), min(punoGolova.calculate(klub2.golovi), punoAvgGolova.calculate(klub2.prosekGolovi))),
                min(punoPobeda.calculate(klub2.pobede), min(punoGolova.calculate(klub2.golovi), punoAvgGolova.calculate(klub2.prosekGolovi)))
            )
        )
    ), debugMode);

    if (debugMode)
        cout << "\nParametri defazifikacije tima 1:\n";
    double crispWRKlub1 = tim1Space.getCOG(debugMode);

    if (debugMode)
        cout << "\nParametri defazifikacije tima 2:\n";
    double crispWRKlub2 = tim2Space.getCOG(debugMode);

    double scalingFactor = 100 / (crispWRKlub1 + crispWRKlub2);

    if (debugMode)
        cout << "\nScaling factoir: " << scalingFactor << endl << "scalingFactor sluzi da pretvori crisp izlaz u procente pogodne za prikaz." << endl <<
        "Crisp izlaz nakon defazifikacije je ocena tima." << endl << endl;

    crispWRKlub1 = crispWRKlub1 * scalingFactor;
    crispWRKlub2 = crispWRKlub2 * scalingFactor;

    char* percent = new char[200];

    string helper;

    helper = to_string(crispWRKlub1) + "% vs. " + to_string(crispWRKlub2) + "%";

    std::strcpy(percent, _strdup(helper.c_str()));
    return percent;
}

int main()
{
    bool work = true;
    while (work)
    {
        system("cls");
        char scan = 'n';
        cout << "Da li zelite da kopirate aktuelne podatke sa sajta Mozzartbet.com?(d/n) ";
        cin >> scan;

        if (scan == 'd')
        {
            cout << "Skeniranje timova..." << endl;

            thread pyWork(runWebScraper);
            pyWork.join();
        }

        fstream textResoults;
        textResoults.open("Findings.txt", ios::in);

        if (textResoults.is_open())
        {
            vector<Klub> klubovi;
            string tempLine;
            while (getline(textResoults, tempLine))
            {
                char* array = _strdup(tempLine.c_str());
                char* token = strtok(array, "|");
                Klub tempKlub;
                int counter = 0;
                while (token != NULL)
                {
                    if (counter == 1)
                        tempKlub.naziv = token;
                    else if (counter == 2)
                        tempKlub.pobede = atoi(token);
                    else if (counter == 3)
                        tempKlub.golovi = atoi(token);
                    else if (counter == 4)
                        tempKlub.prosekGolovi = stod(token);

                    token = strtok(NULL, "|");
                    counter++;
                }

                klubovi.push_back(tempKlub);
            }

            cout << "\n\n Debug mode?(d/n): ";
            char debugConfirm = 'n';
            cin.clear();
            cin >> debugConfirm;

            cout << "\n\nProcenti pobede:\n\n";

            for (int i = 0; i < klubovi.size(); i += 2) 
            {
                if (!klubovi[i].hasData() || !klubovi[i + 1].hasData())
                {
                    cout << klubovi[i].naziv << " vs. " << klubovi[i + 1].naziv << " | " << "50% / 50% (nema podataka o mecu)" << endl << endl;
                    continue;
                }

                char* percentage = getPercentage(klubovi[i], klubovi[i + 1], debugConfirm == 'd' || debugConfirm == 'D' ? true : false);
                cout << klubovi[i].naziv << " vs. " << klubovi[i + 1].naziv << " | " << percentage << endl << endl;
                delete[] percentage;
            }
        }
        else
        {
            cout << "Greska..." << endl;
            continue;
        }
        textResoults.close();

        cout << "--------------------------------------------------------------------\n";
        cout << "Da li zelite da pokusate ponovo?(d/n) ";
        cin >> scan;

        if (scan != 'd')
            work = false;
    }

    cout << "\n\nTvorac ovog programa nije odgovoran za gubitke/dobitke pri kockanju, cinite to na sopstvenu odgovornost!!\n\n";
    system("pause");
    return 0;
}
