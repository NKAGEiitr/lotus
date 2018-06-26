#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "TLine.h"
#include "TText.h"
#include "TPaveText.h"
#include "TLegend.h"
#include "TEllipse.h"
#include "TH1D.h"

#include "Utils.h"
#include "Substrate.h"
#include "Droplet.h"
#include "MDBase.h"
#include "Quiver.h"
#include "FieldViz.h"
#include "Fitting.h"

using namespace std;

///////////////////
// Visualization //
///////////////////


class Figure {
 protected:
  TCanvas* canvas = NULL;
  TLegend* legend = NULL;
  string title;
  int width;
  int height;
  SimData* simDataPtr = NULL;
  Options options;
  double xlo, xhi;
  double ylo, yhi;
  char outputDir[256];

 public:
  Figure(const string _title, SimData &simData);
  ~Figure();

  void createCanvas();
  void setCanvasStyle();
  void setOutputDir(const char* path);
  void saveImage();
  void saveROOT();
  void save(const char* filename);
};

class DropletFigure : public Figure {
  Droplet* dropletPtr = NULL;

  TH2D* hDroplet = NULL;
  TEllipse* eCircle = NULL;
  TGraph* gCirclePoints = NULL;
  CircleFit* circlePtr = NULL;

  TLine* tangentLine = NULL;
  TLine* bulkEdgeLine = NULL;
  TLine* monoEdgeLine = NULL;
  TLine* heightLine = NULL;
  TLine* monoHiLine = NULL;
  TLine* monoLoLine = NULL;

  TPaveText* textBox = NULL;
  TText* cAText = NULL;
  TText* dHText = NULL;
  TText* bEText = NULL;
  TText* mEText = NULL;
  TText* mHText = NULL;
  TText* mLText = NULL;

  double bulkEdge;
  double monoEdge;
  double dropletHeight;
  double contactAngle;
  double monoLimits[2];

 public:
  DropletFigure(const string _title, Droplet &droplet);
  ~DropletFigure();

  void createLines();
  void createCircle();
  void createLegend();

  void deleteLines();
  void deleteCircle();
  void deleteLegend();

  void setLineStyle();
  void setHistStyle();
  void setCircleStyle();
  void setGraphStyle();
  void setLegendStyle();
  void setStyle();

  void setValues();

  void setLegendText();
  void addLegendEntries();
  void setTitle();
  void setAxisLabels();

  void drawHist();
  void drawLines();
  void drawCircle();
  void drawGraph();
  void drawText();
  void drawLegend();
  void drawAnnotations();
  void draw();
};


class DensFigure : public Figure {
  TLine* monoHiLineDens = NULL;
  TLine* monoLoLineDens = NULL;
  TH1D* hLiquidDens = NULL;
  TH1D* hSubstrateDens = NULL;
  double monoLimits[2];

  Droplet* dropletPtr = NULL;
  Substrate* substratePtr = NULL;

 public:
  // TODO: Set title later
  DensFigure(const string _title, Droplet &droplet, Substrate &substrate);
  ~DensFigure();

  void setTitle();
  void setAxisLabels();
  void setLineStyle();
  void setLegendStyle();
  void setStyle();

  void setValues();

  void createLines();
  void createLegend();

  void deleteLines();
  void deleteLegend();

  void drawHists();
  void drawLines();
  void drawLegend();
  void draw();
};

class TanhFigure : public Figure {
  TGraph* gPoints = NULL;

  TLine* ldLine = NULL;
  TLine* halfLdLine = NULL;
  TLine* x0Line = NULL;

  TPaveText* tanhTextBox = NULL;
  TText* posText = NULL;

  double ld;
  double w_guess;
  double w;
  double x0;
  double x0_guess;
  double lowGuess;
  double hiGuess;
  double startPoint;
  double val;

  char rowOrCol[4];
  int rowColNum;

  static const int numLines = 3;
  static const int numTexts = 5;

  TText* tanhTexts[numTexts];
  TLine* tanhLines[numLines] = {
    ldLine,
    halfLdLine,
    x0Line,
  };

  TanhFit* tanhFitPtr = NULL;

 public:
  TanhFigure(const string _title, TanhFit &tanhFit);
  ~TanhFigure();

  void createLines();
  void createGraph();
  void createLegend();
  void deleteLines();
  void deleteGraph();
  void deleteLegend();

  void addLegendEntries();
  void setValues();
  void setText();
  void setAxisLabels();
  void updateRowCol();

  void setTitle();
  void setLinePositions();

  void setHistStyle();
  void setLineStyle();
  void setGraphStyle();
  void setLegendStyle();
  void setStyle();

  void drawHist();
  void drawFunction();
  void drawLines();
  void drawGraph();
  void drawLegend();
  void draw();
};

#endif
