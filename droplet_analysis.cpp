//Read each timestep and plot z(r)

#include "TCanvas.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TLegend.h"
#include "TMultiGraph.h"
#include "TF1.h"
#include "TF2.h"
#include "TFile.h"
#include "TText.h"
#include "TPaveText.h"


#include "Utils.h"
#include "Atoms.h"
#include "Droplet.h"
#include "MDBase.h"
#include "Parameters.h"
#include "Readers.h"
#include "Time.h"
#include "Visualization.h"


using namespace std;

const double PI = 3.141592653589793;

//Read each timestep and plot 3d scatter plot
int main(int argc,char* argv[])
{
    //Command line arguments: inLoc,outLoc
    cout << "Received " << argc-1 << " command line arguments." << endl;

    char* inLoc=argv[1];
    char* outLoc=argv[2];

    //Whether to run whole analysis or only find monolayer
    bool onlyFindInterface;
    cout << "argc=" << argc << endl;
    if(argc<4)
        onlyFindInterface=false;
    else
        onlyFindInterface=(strcmp(argv[3],"mono")==0);

    /*
    cout << "argv: " << endl;
    for(int i=0;i<argc;i++)
        cout << argv[i] << endl;
    cout << endl;
    */

    //z limits of the monolayer
    double monoLimits[2]={14.5,15.5};
    double monoWidth=monoLimits[1]-monoLimits[0];

    //While reading files, we don't yet know where the monolayer will be defined for this frame, so we save an atom's coordinates if it's in a broad range, then eliminate extras later.
    //double potentialMonoLimits[2]={14.5,15.5};
    //int nMonoAtoms=0;
    //int nPotentialMonoAtoms=0;
    //vector<double> potentialMonoR;
    //vector<double> potentialMonoZ;

    //Open monoLimits file
    /*
    fstream interfaceFile;
    if(!onlyFindInterface)
    {
        interfaceFile.open("../mono_limits.txt",ios::in);
    }
    else
        interfaceFile.open("../mono_limits.txt",ios::out);


    if (interfaceFile.good())
        cout << "Successfully opened " << "mono_limits.txt" << endl;
    else
        cout << "Failed to open " << "mono_limits.txt" << endl;

    if(!onlyFindInterface)
    {
        interfaceFile >> monoLimits[0] >> monoLimits[1];
        cout << "monoLimits=" << monoLimits[0] << " " << monoLimits[1] << endl;
    }
    */

    //File Number
    string fileName(inLoc);
    string halfName=fileName.substr(fileName.find("atom")).substr(4);
    string fileNumberStr=halfName.substr(0,halfName.find("/"));
    stringstream fileNumberSS;
    fileNumberSS << fileNumberStr;
    int fileNumber;
    fileNumberSS >> fileNumber;
    cout << "Reading file number " << fileNumber << endl;

    cout << "Open Stream" << endl;
    //Input Stream
    ifstream inFile(inLoc);

    //Substrate density input file
    string dStr(argv[1]);
    stringstream dSS;
    dSS << dStr.substr(0,dStr.find("calculated.txt")) << "substrate_density_halfA.txt";
    cout << "Opening " << dSS.str().data() << endl;
    ifstream densFile(dSS.str().data());

    //Center of mass file
    ifstream centerFile("center_of_mass.txt");

    //Check files
    if (inFile.good())
        cout << "Successfully opened " << inLoc << endl;
    else
    {
        cout << "Failed to open " << inLoc << endl;
        return 1;
    }

    if (densFile.good())
        cout << "Successfully opened " << "substrate_density_halfA.txt" << endl;
    else
    {
        cout << "Failed to open " << "substrate_density_halfA.txt" << endl;
        return 1;
    }

    if(centerFile.good())
        cout << "Successfully opened center_of_mass.txt" << endl;
    else
    {
        cout << "Failed to open center_of_mass.txt" << endl;
        return 1;
    }

    //Substrate density variables
    int histDensBin;
    string densLine;
    double density;
    //Read first line (header comment)
    getline(densFile,densLine);

    //Split Header comment into vector of strings
    vector<string> densHeader = strSplit(densLine);

    //Count number of fields & allocate zVals array
    int nDensBinsFile = densHeader.size()-1;
    double *zVals = new double[nDensBinsFile];

    cout << "nDensBinsFile=" << nDensBinsFile << endl;
    //Remove "z=" from the beginning of each label and convert to decimal
    for(int i=0;i<nDensBinsFile;i++)
    {
        zVals[i]=atof(densHeader[i+1].substr(2).data()); //substr(2) means char 2 (counting from 0 - actually 3rd character) until the end
    }
    double dLoFile=zVals[0];
    double dZDens=zVals[1]-zVals[0];
    double dHiFile=zVals[nDensBinsFile-1]+dZDens;

    //Density histogram limits - dz for file & hist should be same, everything else can be different
    double dLoHist=0;
    double dHiHist=60;
    int nDensBinsHist=(int) floor((dHiHist-dLoHist)/dZDens);
    cout << "nDensBinsHist=" << nDensBinsHist << endl;

    //Ignore blank line
    getline(densFile,densLine);

    //Output file streams
    FILE* avgStepData = fopen("avgStepData.txt","w");
     fprintf(avgStepData,"%8.8s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s\n","#time","bulkEdge","monoEdge","contactAngle","dropletHeight","avgRadius","avgDipole","MSDx","MSDy","MSDz","MSDavg","frameFlux","dMe_dt","nMono","chi2s");
    fflush(avgStepData);

    FILE* instStepData = fopen("instStepData.txt","w");
     fprintf(instStepData,"%8.8s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s %15.15s \n","#time","avgRadius","avgDipole","MSDx","MSDy","MSDz","MSDavg","stepFlux");
    fflush(instStepData);

    //Variables
    string line;
    string input;

    int atomNum=0;
    int lineNum=1;

    double coords[3];
    double velocities[3];
    double dipole;

    int timestep; //Real step number for simulation
    int stepNum=1; //Step number relative to this file starting with 1
    int frameNum=0;
    int frameStep; //First real timestep # of frame - used for naming, plots, etc.
    bool frameNamed=false; //Whether this frame has been named yet

    bool loopFlag1=true;
    bool loopFlag2=true;
    bool skipToEnd=false; //If true, skip first 485 steps

    //Find out some things about the file
    cout << endl;
    cout << "Analyzing input file" << endl;
    ifstream testFile(inLoc);
    int numSteps=countSteps(testFile);
    int numAtoms=countAtoms(testFile);
    int firstID=lowestID(testFile);
    testFile.close();

    //Number of timesteps to average together to make a frame
    int stepsPerFrame=5;

    //Is the number of steps divisible by the number of steps per frame?
    bool extraSteps = numSteps%stepsPerFrame;
    bool divisible = (extraSteps==0);

    //End of the penultimate frame (or middle of last frame if not divisible)
    int penultimateFrame = numSteps-extraSteps;

    cout << "extraSteps: " << extraSteps << endl;
    cout << "divisible: " << divisible << endl;
    cout << "penultimateFrame: " << penultimateFrame << endl;

    //Number of frames (collections of timesteps)
    //If not divisible, then the last frame will have more than the rest
    int numFrames=(int) floor(numSteps/stepsPerFrame);

    //Coordinates
    vector<double> x(numAtoms);
    vector<double> y(numAtoms);
    vector<double> z(numAtoms);
    vector<double> r(numAtoms);
    vector<double> p(numAtoms);

    vector<double> vx(numAtoms);
    vector<double> vy(numAtoms);
    vector<double> vz(numAtoms);
    vector<double> vr(numAtoms);

    vector<double> cosT(numAtoms);

    //Initial positions
    vector<double> xi(numAtoms);
    vector<double> yi(numAtoms);
    vector<double> zi(numAtoms);

    //Read from file
    bool initWritten=file_exists("../init.txt");
    if (initWritten)
    {
        ifstream initIn("../init.txt");
        for(int i=0;i<numAtoms;i++)
        {
            initIn >> xi[i] >> yi[i] >> zi[i];
            initIn.ignore(256,'\n');
        }
        initIn.close();
    }

    //Center of mass
    double x0,y0,z0;
    bool centroidKnown;

    //Radius of cylinder to use to calculate density(z) of water
    //Use half of the radius of the droplet
    int simPos=0; //When submitted by analyze.sh, whole path is included in argv[2]
    if(string(argv[2]).length()>40)
        simPos=44;
    double rDensCyl=atoi(string(argv[2]).substr(simPos,2).data())/2;
    cout << "RDENSCYL=" << rDensCyl << endl;

    //Step variables
    double rBulkMax=rDensCyl*2;
    double bulkEdge=rDensCyl*2;
    double tmpBulkEdge; //throwaway
    double monoEdge;
    double lastMonoEdge=0;
    double dropletHeight,contactAngle;
    double avgRadius,avgDipole;
    vector<double> msd;
    //double exchange;
    double min;
    int nMono;
    int bin1,bin2;

    //Flux for this timestep
    int stepFlux = 0;
    //Average flux for frame
    int frameFlux = 0;

    //Hist of where atoms join the monolayer
    TH1D* rScaledJoin = new TH1D("rScaledJoin","rScaledJoin",30,0,1.5);

    //Hist of where atoms leave the monolayer
    TH1D* rScaledLeave = new TH1D("rScaledLeave","rScaledLeave",30,0,1.5);

    double dz=1.5;
    double zlo=10;
    double zhi=142; //142 for 60A, 121 for others
    int nz=(int) round((zhi-zlo)/dz);

    //Check that dz, zlo, and zhi are compatible
    if( !(fmod((zhi-zlo),dz)==0) )
    {
        cout << "(zhi-zlo) is not divisible by dz" << endl;
        return 1;
    }

    cout << "nz=" << nz << endl;

    const double dA=500;
    const double alo=0;
    const double ahi=125500;
    int nA=(int) round((ahi-alo)/dA);

    double dr=dz;
    double rlo=sqrt(alo/PI);
    double rhi=sqrt(ahi/PI);
    int nr=(int) round((rhi-rlo)/dr);

    double vlo=0;
    double vhi=1e7;
    int nV=(int) round((vhi-vlo)/dA);

    double dp=dr;
    double plo=pow((3*vlo/(4*PI)),(1.0/3));
    double phi=pow((3*vhi/(4*PI)),(1.0/3));
    int  np=(int) round((phi-plo)/dp);

    //Bin volume
    double dV=dA*dz;

    //Check that dA, alo, and ahi are compatible
    if( !(fmod((ahi-alo),dA)==0) )
    {
        cout << "(Ahi-Alo) is not divisible by dA" << endl;
        return 1;
    }

    cout << "nA=" << nA << endl;

    //Skip first 2 lines
    for(int i=0;i<2;i++) inFile.ignore(256,'\n');
    lineNum+=2;

    //Fix canvas height & scale width to preserve aspect ratio
    //Added values are to account for window decorations which ROOT for some reason considers
    //Height and width must be divisible by 16 to play nice with ffmpeg
    int cH=1200+28;
    int cW=(int) roundUp((int)floor(cH*(rhi-rlo)/(zhi-zlo)),16)+4;
    cout << "cW=" << cW%16 << endl;
    TCanvas *cD = new TCanvas("cD","cD",cW,cH); //Dipole
    TCanvas *cA = new TCanvas("cA","cA",cW,cH); //2D density plot
    TCanvas *cQ = new TCanvas("cQ","cQ",cW,cH); //Quiver
    TCanvas *cVr = new TCanvas("cVr","cVr",cW,cH); //Plot v_r(z)
    TCanvas *cDens = new TCanvas("cDens","cDens",cW,cH); //Plot density of substrate and water

    TCanvas *cAll = new TCanvas("cAll","cAll",cW,cH); //All 4 plots together
    cAll->Divide(2,2,0,0);

    //Values to use for equal-area bins
    double aVals[nA];
    double vVals[nA];
    double rVals[nA];
    double pVals[nA];
    for(int i=0;i<=nA;i++)
    {
        aVals[i]=i*dA;
        vVals[i]=i*dV;
        rVals[i]=sqrt(aVals[i]/PI);
        pVals[i]=pow(((3*vVals[i])/(4*PI)),1.0/3);
    }

    //Create Histograms
    TH2D *hA = new TH2D("hA","hA",nA,rVals,nz,zlo,zhi);
    TH1D *hMono = new TH1D("hMono","hMono",nA,rVals); //Only monolayer atoms - for calculating monoEdge
    //TH1D *hMonoProj;
    TH1D *hD = new TH1D("hD","hD",20,0,1);
    TH1D *hVr = new TH1D("hVr","hVr",nz,zlo,zhi); //For tracking v_r(z)
    TH1D *hZ = new TH1D("hZ","hZ",nz,zlo,zhi); //For counting # of atoms in each z bin
    TH1D *hWaterDens = new TH1D("hWaterDens","hWaterDens",nDensBinsHist,dLoHist,dHiHist);
    TH1D *hSubstrDens = new TH1D("hSubstrDens","hSubstrDens",nDensBinsHist,dLoHist,dHiHist);

    //misc plot settings
    hA->SetStats(0);

    hVr->SetStats(0);
    hVr->SetLineWidth(2);
    hVr->GetXaxis()->SetTitle("z (#AA)");
    hVr->GetXaxis()->CenterTitle();
    hVr->GetYaxis()->SetTitle("v_{r} (#AA/fs)");
    hVr->GetYaxis()->CenterTitle();

    //Use OpenGL for antialiasing
    gStyle->SetCanvasPreferGL(true);

    //Quiver
    Quiver *q = new Quiver(nr/2,rlo,rhi,nz/2,zlo,zhi);
    q->SetArrowParams(30.0,0.01,2,0.05);
    q->SetLevels(0,2e-3);

    //Axis labels
    hA->GetXaxis()->SetTitle("r (#AA)");
    hA->GetXaxis()->CenterTitle();
    hA->GetYaxis()->SetTitle("z (#AA)");
    hA->GetYaxis()->CenterTitle();

    hD->GetXaxis()->SetTitle("cos#theta (rad)");
    hD->GetXaxis()->CenterTitle();
    hD->GetYaxis()->SetTitle("Occurrence");
    hD->GetYaxis()->CenterTitle();

    hWaterDens->GetXaxis()->SetTitle("z (#AA)");
    hWaterDens->GetXaxis()->CenterTitle();
    hWaterDens->GetYaxis()->SetTitle("#rho (g/cm^{3})");
    hWaterDens->GetYaxis()->CenterTitle();

    //Circle fit graph
    TGraph *circlePointsGraph = new TGraph();
    //TGraph2D *circlePointsGraph = new TGraph2D();

    //////////////////////
    // HIST ANNOTATIONS //
    //////////////////////

    //Bulk & Mono radius vertical lines
    TLine *bulkEdgeLine = new TLine(0,zlo,0,zhi); //Bulk edge
    TLine *monoEdgeLine = new TLine(0,zlo,0,zhi); //Mono edge
    TLine *heightLine = new TLine(0,zlo,rhi,zhi); //Droplet height
    TLine *monoHiLine = new TLine(0,zlo,rhi,zhi); //top of monolayer
    TLine *monoLoLine= new TLine(0,zlo,rhi,zhi); //bottom of monolayer

    //Density plot
    TLine *monoHiLineDens = new TLine(zlo,0,zhi,6); //top of monolayer
    TLine *monoLoLineDens = new TLine(zlo,0,zhi,6); //bottom of monolayer

    //Line properties
    bulkEdgeLine->SetLineWidth(3);
    bulkEdgeLine->SetLineColor(kGreen);
    monoEdgeLine->SetLineWidth(3);
    monoEdgeLine->SetLineColor(kRed);
    heightLine->SetLineWidth(3);
    heightLine->SetLineColor(kOrange+3);
    monoHiLine->SetLineWidth(3);
    monoHiLine->SetLineColor(kBlue);
    monoHiLine->SetLineStyle(9);
    monoLoLine->SetLineWidth(3);
    monoLoLine->SetLineColor(kBlue);
    monoLoLine->SetLineStyle(2);

    //Density Plot
    monoHiLineDens->SetLineWidth(3);
    monoHiLineDens->SetLineColor(kRed);
    monoLoLineDens->SetLineWidth(3);
    monoLoLineDens->SetLineColor(kGreen);

    //2D Density Hist Legend
    TLegend *hALegend = new TLegend(.65,.65,.85,.85);
    hALegend->AddEntry(circlePointsGraph,"Droplet boundary","lp");
    hALegend->AddEntry(bulkEdgeLine,"Bulk radius","l");
    hALegend->AddEntry(monoEdgeLine,"Mono radius","l");
    hALegend->AddEntry(heightLine,"Droplet height","l");
    hALegend->AddEntry(monoHiLine,"Mono top","l");
    hALegend->AddEntry(monoLoLine,"Mono bottom","l");

    //Text to show data
    TPaveText *textBox = new TPaveText();
    textBox->SetX1NDC(.65);
    textBox->SetY1NDC(.5);
    textBox->SetX2NDC(.85);
    textBox->SetY2NDC(.625);

    TText *cAText=textBox->AddText("Contact angle"); //Contact angle
    TText *dHText=textBox->AddText("Droplet height"); //Droplet height
    TText *bEText=textBox->AddText("Bulk edge"); //Bulk edge
    TText *mEText=textBox->AddText("Mono edge"); //Mono edge

    textBox->SetShadowColor(0);
    textBox->SetTextSize(0.025);

    //////////////////////
    // TANH ANNOTATIONS //
    //////////////////////

    //Lines (There are 7)
    TLine *ldTanhLine = new TLine(rlo,zlo,rhi,zlo); //Liquid density (horizontal)
    TLine *solTanhLine = new TLine(0,zlo,0,zhi); //Final solution
    TLine *x0TanhLine = new TLine(0,zlo,0,zhi); //x0 final
    TLine *x0guessTanhLine = new TLine(0,zlo,0,zhi); //x0 guess
    TLine *lowGuessTanhLine = new TLine(0,zlo,0,zhi); //Lower edge of boundary
    TLine *hiGuessTanhLine = new TLine(0,zlo,0,zhi); //Upper edge of boundary
    TLine *lowBinTanhLine = new TLine(0,zlo,0,zhi); //Lowest bin used

    //Line properties
    ldTanhLine->SetLineColor(kYellow);
    ldTanhLine->SetLineWidth(3);
    solTanhLine->SetLineColor(kBlack);
    solTanhLine->SetLineWidth(3);
    x0TanhLine->SetLineColor(kBlue);
    x0TanhLine->SetLineWidth(3);
    x0guessTanhLine->SetLineColor(kCyan);
    x0guessTanhLine->SetLineWidth(3);
    lowGuessTanhLine->SetLineColor(kViolet);
    lowGuessTanhLine->SetLineWidth(3);
    hiGuessTanhLine->SetLineColor(kGreen);
    hiGuessTanhLine->SetLineWidth(3);
    lowBinTanhLine->SetLineColor(kOrange+7);
    lowBinTanhLine->SetLineWidth(3);

    //Tanh Legend
    TLegend *tanhLegend = new TLegend(.65,.65,.85,.85);
    tanhLegend->AddEntry(ldTanhLine,"ld","l");
    tanhLegend->AddEntry(solTanhLine,"sol","l");
    tanhLegend->AddEntry(x0TanhLine,"x0","l");
    tanhLegend->AddEntry(x0guessTanhLine,"x0guess","l");
    tanhLegend->AddEntry(lowGuessTanhLine,"lowGuess","l");
    tanhLegend->AddEntry(hiGuessTanhLine,"hiGuess","l");
    tanhLegend->AddEntry(lowBinTanhLine,"lowBin","l");

    //Text to show data
    TPaveText *tanhTextBox = new TPaveText();
    tanhTextBox->SetX1NDC(.65);
    tanhTextBox->SetY1NDC(.45);
    tanhTextBox->SetX2NDC(.85);
    tanhTextBox->SetY2NDC(.625);

    //Text lines (There are 5)
    TText *tanhTexts[5];
    tanhTexts[0]=tanhTextBox->AddText("pos: ");
    tanhTexts[1]=tanhTextBox->AddText("w_guess: ");
    tanhTexts[2]=tanhTextBox->AddText("w: ");
    tanhTexts[3]=tanhTextBox->AddText("x0_guess: ");
    tanhTexts[4]=tanhTextBox->AddText("x0: ");

    tanhTextBox->SetShadowColor(0);
    tanhTextBox->SetTextSize(0.025);

    //Create array of TLines to pass to solveTanhFit
    TLine *tanhLines[7] = {ldTanhLine,solTanhLine,x0TanhLine,x0guessTanhLine,lowGuessTanhLine,hiGuessTanhLine,lowBinTanhLine};

    /////////////////////////////
    // FieldViz Initialization //
    /////////////////////////////

//    double ff_eps = 5;
//    int lic_iter = 2;
//    double lic_expnt = .5;
//
//    FieldViz flowField(640,480);
//    flowField.SetColormap("/home/oge1/lammps/sapphire/analysis/src/FieldViz/phase_cmap.txt");
//    flowField.SetLICParams(lic_iter, lic_expnt);
//    flowField.Init();
//    flowField.SetBounds(rlo, rhi, zlo, zhi);
//    flowField.SetNAtoms(numAtoms);

    //////////
    // Misc //
    //////////

    //hA Zaxis limits
    double colzMin=0.0;
    double colzMax=1.5;

    //Circle for fitting
    //Use "cut" as second argument for 0.25<dens<0.75 cutoff
    CircleFit Circle(hA);
    double chi2s; //Fitting error

    //Test for hWaterDens
    int nWaterDens=0;
    int nWatersBelowMono=0;

    //Rate of change of monoEdge w.r.t. time.
    double dMe_dt;

    //Determine whether to track mono atoms
    bool trackMonoAtoms=file_exists("../monoIDs.txt");
    int numMonoIDs=0;
    int id;
    vector<int> monoIDs;
    double monoR,monoZ;
    if(trackMonoAtoms)
    {
        cout << "Tracking allocating!" << endl;

        //Read IDs of atoms to track
        ifstream monoIDsIn("../monoIDs.txt");
        while(monoIDsIn.good())
        {
            monoIDsIn >> id;
            monoIDs.push_back((id-firstID)/3);
            numMonoIDs++;
            cout << "ID: " << id << " -> " << (id-firstID)/3 << endl;
        }
        monoIDsIn.close();
    }

    //Declare variables for velocity plot
    double sum,num;

    //Declaring graphs in the if block seems to produce errors
    TGraph* monoGraph1 = new TGraph(numMonoIDs); //Outline
    TGraph* monoGraph2 = new TGraph(numMonoIDs); //Fill

    if(trackMonoAtoms)
    {
        //Create graphs to draw to
        monoGraph1->SetMarkerStyle(20);
        monoGraph1->SetMarkerColor(kBlack);
        monoGraph1->SetMarkerSize(1.6);

        monoGraph2->SetMarkerStyle(20);
        monoGraph2->SetMarkerColor(kWhite);
        monoGraph2->SetMarkerSize(1.3);
    }

    //Create directories if they don't exist

    system("mkdir -p img/hist");
    system("mkdir -p img/mono");
    system("mkdir -p img/quiver");
    system("mkdir -p img/dipole");
    system("mkdir -p img/densityHalfA");
    system("mkdir -p img/vr");
    system("mkdir -p img/vp");
    system("mkdir -p img/all");
    system("mkdir -p img/circles");
    system("mkdir -p img/tanh");
    system("mkdir -p img/lic");
    system("mkdir -p fields");

    //Skip to end if desired (step 486,timestep 970000)
    if(skipToEnd)
    {
        cout << "Skipping to end!" << endl;
        while(lineNum<8374013)
        {
            getline(inFile,line);
            lineNum++;
        }
        stepNum=486;
    }

    ///////////////
    // TIME LOOP //
    ///////////////

    //For each timestep
    while( (stepNum<=numSteps) && (loopFlag1) && (!inFile.eof()))
    {
        //Read the header
        inFile >> line >> timestep;
        inFile.ignore();
        lineNum++;

        cout << endl;
        cout << "Step # " << stepNum << endl;
        cout << "Timestep " << timestep << " @ line " << lineNum-1 << endl;

        loopFlag2=true;

        atomNum=0;

        //Name frame
        if(!frameNamed)
        {
            frameStep=timestep;
            frameNamed=true;
        }

        //Read atom data
        while(loopFlag2)
        {
            getline(inFile,line);
            lineNum++;
            if(line=="") loopFlag2=false;
            else
            {
                //Save data
                strToData(coords,velocities,dipole,line);
                x[atomNum]=coords[0];
                y[atomNum]=coords[1];
                z[atomNum]=coords[2];

                vx[atomNum]=velocities[0];
                vy[atomNum]=velocities[1];
                vz[atomNum]=velocities[2];

                cosT[atomNum]=dipole;
                atomNum++;
            }
        }

        //Save first timestep
        if( (!initWritten) && (stepNum==1) )
        {
            ofstream initOut("../init.txt");
            for(int i=0;i<numAtoms;i++)
            {
                xi[i]=x[i];
                yi[i]=y[i];
                zi[i]=z[i];

                //Save to file
                initOut << x[i] << " " << y[i] << " " << z[i] << endl;
            }
            initOut.close();
        }

        //Test - why is the last timestep repeated in instStepData.txt?
        cout << "Last line read: " << lineNum << endl;

        //Center of mass

        //Read center of mass file
        //Ignore timestep
        centerFile.ignore(256,' ');
        //Get center of mass
        centerFile >> x0 >> y0 >> z0;
        //Go to next line
        centerFile.ignore(256,'\n');

        //Use droplet xy center and substrate z center
        x0=mean(x);
        y0=mean(y);

        //Report findings
        cout << "{x0,y0,z0}={" << x0 << "," << y0 << "," << z0 << "}" << endl;

        /*
        if(!centroidKnown)
        {
            x0=mean(x);
            y0=mean(y);
            centroidKnown=true;

            if(file_exists("../centroid.txt"))
            {
                ifstream centroidIn("../centroid.txt");
                centroidIn >> x0 >> y0;
                centroidIn.close();
            }

            else
            {
                ofstream centroidOut("../centroid.txt");
                centroidOut << x0 << " " << y0;
                centroidOut.close();
            }

        }
        */

        //Minimum
        min=findMinimum(z)-z0;

        //Track mono atoms
        if(trackMonoAtoms)
        {
            for(int i=0;i<numMonoIDs;i++)
            {
                monoR=sqrt(square(x[monoIDs[i]]-x0)+square(y[monoIDs[i]]-y0));
                monoZ=z[monoIDs[i]]-z0;
                //Fill graphs with radial positions of mono atoms
                monoGraph1->SetPoint(i,monoR,monoZ);
                monoGraph2->SetPoint(i,monoR,monoZ);
            }
        }


        /*
        cout << "convFact=" << convFact << endl;
        cout << "dA=" << dA << endl;
        cout << "dV=" << dV << endl;
        */

        //Fill Histograms
        for(int i=0;i<numAtoms;i++)
        {
            //Subtract center of mass from x,y,z coordinates
            x[i]-=x0;
            y[i]-=y0;
            z[i]-=z0;

            //Calculate radius
            r[i]=sqrt(square(x[i])+square(y[i]));
            p[i]=sqrt(square(r[i])+square(z[i]-zlo));

            vr[i]=(x[i]*vx[i]+y[i]*vy[i])/r[i]; // Chain rule

            hA->Fill(r[i],z[i],convFact/dV);
            hD->Fill(dipole,1/numAtoms);
            hVr->Fill(z[i],vr[i]);
            hZ->Fill(z[i]);
            q->Fill(r[i],z[i],vr[i],vz[i]);

            //Fill water density histogram
            //water=18amu, convert units, divide by volume
            if(r[i]<=rDensCyl)
            {
                hWaterDens->Fill(z[i],convFact/(dZDens*PI*rDensCyl*rDensCyl));
                nWaterDens++;
            }

            //Fill monolayer histogram
            if((monoLimits[0]<=z[i]) && (z[i]<=monoLimits[1]))
            {
                hMono->Fill(r[i],convFact/(dA*monoWidth));
                //nPotentialMonoAtoms++;
                //potentialMonoR.push_back(r[i]);
                //potentialMonoZ.push_back(z[i]);
            }
            else if(z[i]<monoLimits[0])
            {
                cout << "MOLECULE BELOW MONO THRESHOLD AT Z=" << z[i] << endl;
                nWatersBelowMono++;
            }
        }

        if(nWatersBelowMono>0)
            cout << "Found " << nWatersBelowMono << " waters below monolayer in step " << timestep << endl;
        nWatersBelowMono=0;

        //Fill Density histogram
        //Ignore timestep
        densFile >> densLine;
        for(int i=0;i<nDensBinsFile;i++)
        {
            densFile >> density;
            histDensBin = (int) floor((zVals[i]-dLoHist)*nDensBinsHist/(dHiHist-dLoHist))+1;
            //cout << "zVals[" << i << "]=" << zVals[i] << endl;
            //cout << (zVals[i]-dLoFile)*nDensBinsHist/(dHiHist-dLoHist) << endl;
            //cout << "Density " << zVals[i] << ": " << histDensBin << " = " << density << endl;
            hSubstrDens->SetBinContent(histDensBin,density);
        }
        //Go to next line
        densFile.ignore(256,'\n');

        //INST CALCULATIONS

        //Calculate average radius
        avgRadius=mean(r);

        //Calculate average dipole moment angle
        avgDipole=mean(cosT);

        //Calculate MSD
        msd=calculateMSD(xi,yi,zi,x,y,z);

        //Update lists of where atoms joined and left the monolayer
        stepFlux = monoFlux(r,z,monoLimits,bulkEdge,rScaledJoin,rScaledLeave,nMono);

        cout << "Step Flux: " << stepFlux << endl;

        //Calculate average flux for frame (gets reset after written every frame)
        frameFlux += stepFlux;

        //WRITE INST
        fprintf(instStepData,"%08d %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15d\n",timestep,avgRadius,avgDipole,msd[0],msd[1],msd[2],msd[3],stepFlux);

        ///////////////////////
        // FieldViz add data //
        ///////////////////////

//        flowField.SetData(&r[0], &z[0], &vr[0], &vz[0], numAtoms);
//        flowField.AddFieldFromData(ff_eps);

        //FRAME CALCULATIONS (5 timesteps or last step)

        //The last frame may contain more timesteps than other frames (generally 6, up to 9).
        //So if numSteps is not divisible by stepsPerFrame, then the following should not run for what would be the penultimate frame, and it should be combined with the remaining steps to form the final frame.
       //stepsPerFrame may need to be altered for the last timestep to correct some calculations.
        if( ((stepNum%stepsPerFrame==0)&&!(!divisible&&(stepNum==penultimateFrame))) || (stepNum==numSteps&&!divisible) )
        {
            cout << endl;
            cout << "///////////////" << endl;
            cout << "// FRAME " << setw(3) << stepNum/stepsPerFrame << " //" << endl;
            cout << "///////////////" << endl;
            cout << endl;
            cout << "frameStep: " << frameStep << endl;
            //stepsPerFrame needs to be altered on the last frame if it has extra steps
            if(!divisible && stepNum==numSteps)
                stepsPerFrame+=extraSteps;
            cout << stepsPerFrame << " steps this frame." << endl;
            cout << endl;

            //FRAME CALCULATIONS

            //Scale by stepsPerFrame since this is a frame averege
            hWaterDens->Scale(1.0/stepsPerFrame);
            hA->Scale(1.0/stepsPerFrame);
            hMono->Scale(1.0/stepsPerFrame);

            //Find monolayer
            //findMonoLimits(hWaterDens,monoLimits);

            /*
            //Write monolayer to file if appropriate
            if(onlyFindInterface)
            {
                interfaceFile << monoLimits[0] << " " << monoLimits[1];
                loopFlag1=false;
            }
            */

            /*
            //Determine which atoms are really in the monolayer
            for(int i=0;i<nPotentialMonoAtoms;i++)
            {
                if((monoLimits[0]<=potentialMonoZ[i]) && (z[i]<=potentialMonoR[1]))
                {
                    hMono->Fill(potentialMonoR[i],convFact/(dA*monoWidth));
                    nMonoAtoms++;
                }
            }
            */

            //Report findings
            //cout << "Found " << nPotentialMonoAtoms << " potentially in monolayer" << endl;
            //cout << " Of them, " << nMonoAtoms << " are really in the monolayer." << endl;

            /*
            //Reset potential monolayer variables
            nMonoAtoms=0;
            nPotentialMonoAtoms=0;
            potentialMonoR.clear();
            potentialMonoZ.clear();
            */

            cout << "Find Boundary" << endl;

            //Do Tanh fitting, find monolayer radius
            findBoundaryPoints(hA,circlePointsGraph,"a",monoLimits,hMono,rBulkMax,monoEdge,bulkEdge,frameStep,tanhLegend,tanhLines,tanhTexts,tanhTextBox);
            cout << "Mono edge: " << monoEdge << endl;

            cout << "Fit Circle" << endl;
            //Fit circle
            cA->cd();
            chi2s=fitCircle(circlePointsGraph,Circle,rBulkMax,frameStep);
            cout << "chi2s_ps:" << chi2s << endl;

            cout << "Find radii" << endl;

            //Find bulk radius
            // Previously using value from first bulk row
            // Now, just throw this away and use the tanhfit val from
            // first bulk row, which has already been found
            // in findBoundaryPoints
            tmpBulkEdge=Circle.Intersect(monoLimits[1]);
            cout << "Bulk edge: " << bulkEdge << endl;

            //Find contact angle
            cout << "Intersect with " << monoLimits[1] << endl;
            contactAngle=Circle.ContactAngle();

            //Find droplet height by intersecting circle with y-axis
            dropletHeight=Circle.Height();

            //Test number of molecules in hWaterDens
            cout << "STEP " << timestep << ": " << nWaterDens << " molecules in hWaterDens" << endl;
            nWaterDens=0;

            //PLOT TITLES
            stringstream title;
            title << string(argv[2]).substr(simPos) << ": "  << frameStep;
            hA->SetTitle(title.str().data());

            title.str("");
            title << "Velocity Field: " << frameStep;
            q->SetTitle(title.str().data());

            title.str("");
            title << "Density: " << frameStep;
            hWaterDens->SetTitle(title.str().data());

            title.str("");
            title << "Dipole Moment Distribution: " << frameStep;
            hD->SetTitle(title.str().data());

            title.str("");
            title << "2D Radial Velocity: " << frameStep;
            hVr->SetTitle(title.str().data());

            //FRAME PLOTS

            //Draw dipole Histogram
            cD->cd();
            hD->Draw();
            title.str("");
            title << "img/dipole/step" << setw(8) << setfill('0') << frameStep << ".png";
            cD->SaveAs(title.str().data());

            //Quiver
            title.str("");
            title << "img/quiver/step" << setw(8) << setfill('0') << frameStep << ".png";
            q->Draw(cQ);
            cout << "Saving quiver" << endl;
            q->SaveAs(title.str().data());

            //Density plot
            title.str("");
            title << "img/densityHalfA/step" << setw(8) << setfill('0') << frameStep << ".png";
            cDens->cd();
            //Drawing options
            hWaterDens->SetLineColor(kBlue);
            hSubstrDens->SetLineColor(kOrange+3); //Brown
            hWaterDens->SetLineWidth(2);
            hSubstrDens->SetLineWidth(2);
            hWaterDens->Draw();
            hSubstrDens->Draw("SAME"); //Same canvas
            //Use OpenGL for antialiasing
            gStyle->SetCanvasPreferGL(true);
            //Axis limits
            hWaterDens->GetYaxis()->SetRangeUser(0,6);
            //Monolayer edges
            monoHiLineDens->SetX1(monoLimits[1]);
            monoHiLineDens->SetX2(monoLimits[1]);
            monoHiLineDens->Draw();
            monoLoLineDens->SetX1(monoLimits[0]);
            monoLoLineDens->SetX2(monoLimits[0]);
            monoLoLineDens->Draw();
            //Legend
            TLegend *densLeg = new TLegend(.75,.75,.85,.85);
            densLeg->AddEntry(hWaterDens,"Water");
            densLeg->AddEntry(hSubstrDens,"Substrate");
            densLeg->AddEntry(monoLoLineDens,"Mono lower limit","l");
            densLeg->AddEntry(monoHiLineDens,"Mono upper limit","l");
            densLeg->Draw();
            //Save
            cDens->SaveAs(title.str().data());

            //2D Velocity Plot
            //Scale values to create average
            for(int i=0;i<nz;i++)
            {
                sum=hVr->GetBinContent(i+1);
                num=hZ->GetBinContent(i+1);
                if(num!=0)
                    hVr->SetBinContent(i+1,abs(sum/num));
            //    cout << "hVr[" << i+1 << "] = " << hVr->GetBinContent(i+1) << endl;
            }
            //Plot
            cVr->cd();
            TGraph *g = horizontalHist(hVr);
            g->GetXaxis()->SetLimits(1e-6,1e-3);
            cVr->SetLogy();
            g->Draw("AP");
            cVr->Update();
            q->Draw(cVr);
            title.str("");
            title << "img/vr/step" << setw(8) << setfill('0') << frameStep << ".png";
            cVr->SaveAs(title.str().data());

            //Draw 2D density histogram
            cA->cd();
            hA->SetMinimum(colzMin);
            hA->SetMaximum(colzMax);
            hA->Draw("colz");
            //Draw circle
            TEllipse *circleEllipse = Circle.Draw();
            //Draw tangent line
            TLine *tangentLine = Circle.DrawTangentLine();
            //Add lines
            bulkEdgeLine->SetX1(bulkEdge);
            bulkEdgeLine->SetX2(bulkEdge);
            bulkEdgeLine->Draw();
            monoEdgeLine->SetX1(monoEdge);
            monoEdgeLine->SetX2(monoEdge);
            monoEdgeLine->Draw();
            heightLine->SetY1(dropletHeight);
            heightLine->SetY2(dropletHeight);
            heightLine->Draw();
            monoHiLine->SetY1(monoLimits[1]);
            monoHiLine->SetY2(monoLimits[1]);
            monoHiLine->Draw();
            monoLoLine->SetY1(monoLimits[0]);
            monoLoLine->SetY2(monoLimits[0]);
            monoLoLine->Draw();
            //Draw circle points graph
            circlePointsGraph->SetMarkerStyle(20);
            circlePointsGraph->Draw("same P");
            //Add legend (once)
            if(frameNum==0)
            {
                hALegend->AddEntry(tangentLine,"Tangent line","l");
            }
            hALegend->Draw();
            //Add data text box
            title.str("");
            title << "Contact angle: " << contactAngle;
            cAText->SetText(0,0,title.str().data());
            title.str("");
            title << "Droplet height: " << dropletHeight;
            dHText->SetText(0,0,title.str().data());
            title.str("");
            title << "Bulk radius: " << bulkEdge;
            bEText->SetText(0,0,title.str().data());
            title.str("");
            title << "Mono radius: " << monoEdge;
            mEText->SetText(0,0,title.str().data());
            textBox->Draw();

            //Name & Save
            stringstream aName;
            aName << "img/hist/abin" << setw(8) << setfill('0') << frameStep << ".png";
            cA->SaveAs(aName.str().data());

            //Delete ellipse and tangent line
            delete circleEllipse;
            delete tangentLine;

            //Draw mono atoms
            if(trackMonoAtoms)
            {
                monoGraph1->Draw("P");
                monoGraph2->Draw("P");

                stringstream mName;
                mName << "img/mono/monoHist" << setw(8) << setfill('0') << frameStep << ".png";
                cA->SaveAs(mName.str().data());
            }

            //Draw all together
            cAll->cd(1);
            gPad->Clear();
            cA->DrawClonePad();

            cAll->cd(2);
            gPad->Clear();
            cVr->DrawClonePad();

            cAll->cd(3);
            gPad->Clear();
            cQ->DrawClonePad();

            title.str("");
            title << "img/all/step" << setw(8) << setfill('0') << frameStep << ".png";

            //FOR SOME REASON, THE PROGRAM CRASHES HERE IF THE FILE IS SAVED
            //cAll->SaveAs(title.str().data());

            cout << "Reset histograms" << endl;
            //Reset histograms
            hA->Reset();
            hMono->Reset();
            hD->Reset();
            q->Reset();
            hWaterDens->Reset();
            hSubstrDens->Reset();

            //Calculate rate of change of monoEdge w.r.t. time (units A/step)
            dMe_dt=(monoEdge-lastMonoEdge)/stepsPerFrame;

            cout << endl;
            cout << "Frame Flux: " << frameFlux << endl;
            cout << "dMe_dt: " << dMe_dt << endl;
            cout << endl;

            //////////////////
            // FieldViz LIC //
            //////////////////

			/*
            title.str("");
            title << "img/lic/lic" << setw(8) << setfill('0') << frameStep << ".ppm";
			stringstream xFile, yFile;
			xFile << "fields/xfield" << setw(8) << setfill('0') << frameStep << ".txt";
			yFile << "fields/yfield" << setw(8) << setfill('0') << frameStep << ".txt";

            flowField.WriteField(xFile.str().data(),yFile.str().data(),"xy");
            flowField.PreProcess();
            flowField.PerformLIC();
            flowField.PostProcess();
            flowField.WriteImage2PPM(title.str().data());
            flowField.ResetField();
			*/

            //Save average data for frame to file
            fprintf(avgStepData,"%08d %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15d %15.6f %15d %15f\n",frameStep,bulkEdge,monoEdge,contactAngle,dropletHeight,avgRadius,avgDipole,msd[0],msd[1],msd[2],msd[3],frameFlux,dMe_dt,nMono,chi2s);
            fflush(avgStepData);
            cout << "Wrote step " << frameStep << " to avgStepData.txt" << endl;

            //Save monolayer radius
            lastMonoEdge=monoEdge;

            //Reset frame flux
            frameFlux = 0;
            frameNum++;

            //Reset name for next frame
            frameNamed = false;

            //Only first frame
            //loopFlag1=false;
        }

        //Only first timestep
        //loopFlag1=false;

        //Increment relative timestep counter
        stepNum++;

    }

    inFile.close();
    fclose(avgStepData);
    fclose(instStepData);

    //Join & Leave
    double I;
    TH1D* rExchange = (TH1D*) rScaledJoin->Clone();
    //Normalize histograms
    rExchange->Add(rScaledLeave,-1);
    I=rScaledJoin->Integral();
    rScaledJoin->Scale(1/I);
    I=rScaledLeave->Integral();
    rScaledLeave->Scale(1/I);
    I=rExchange->Integral();
    rExchange->Scale(1/I);
    TCanvas* cE = new TCanvas();
    cE->cd();
    rScaledJoin->Draw("");
    rScaledLeave->Draw("same");
    rExchange->Draw("same");
    cE->SaveAs("leaveJoinHists.png");

    ofstream leaveJoinFile("leavejoin.txt");
    leaveJoinFile << "#r/R join leave exchange" << endl;
    for(int i=0;i<rScaledJoin->GetXaxis()->GetNbins();i++)
    {
        leaveJoinFile << i*(rScaledJoin->GetBinCenter(2)-rScaledJoin->GetBinCenter(1)) << " " << rScaledJoin->GetBinContent(i+1) << " " << rScaledLeave->GetBinContent(i+1) << " " << rExchange->GetBinContent(i+1) << endl;
    }
    leaveJoinFile.close();

    TFile* LeaveJoin = new TFile("leavejoin.root","RECREATE");
    rScaledJoin->Write("rScaledJoin");
    rScaledLeave->Write("rScaledLeave");
    rExchange->Write("rExchange");
    LeaveJoin->Close();

    delete cE,rScaledJoin,rScaledLeave,rExchange;
    delete hA,q,hD;
    delete hWaterDens,hSubstrDens;
    delete [] zVals;
    delete circlePointsGraph;

    cout << "Done!"<<endl;
    return 0;
}





//Calculate MSD - mean squared displacement
vector<double> calculateMSD(vector<double> xi,vector<double> yi,vector<double> zi,vector<double> x, vector<double> y,vector<double> z)
{
    vector<double> msd(4);

    int numAtoms=xi.size();

    double xSum=0;
    double ySum=0;
    double zSum=0;

    for(int i=0;i<numAtoms;i++)
    {
        xSum+=square(x[i]-xi[i])/numAtoms;
        ySum+=square(y[i]-yi[i])/numAtoms;
        zSum+=square(z[i]-zi[i])/numAtoms;
    }

    msd[0]=xSum;
    msd[1]=ySum;
    msd[2]=zSum;
    msd[3]=(xSum+ySum+zSum)/3;

    return msd;
}

//Count the number of timesteps
int countSteps(ifstream &inFile)
{
    string line;
    int numSteps=0;
    int lineNum=0;

    //Count number of timesteps
    while(getline(inFile,line))
    {
        if(line.find("TIMESTEP")!=string::npos)
            numSteps++;
    }

    //Unset eof flag (if set) & return to beginning of file
    inFile.clear();
    inFile.seekg(0,ios::beg);

    cout << "Counted " << numSteps << " timesteps." << endl;
    return numSteps;
}

//Find the lowest atom ID for a water molecule in this simulation
int lowestID(ifstream &inFile)
{
    int id;

    //Ignore the first 3 lines
    for(int i=0;i<3;i++)
        inFile.ignore(256,'\n');

    //Get ID
    inFile >> id;

    //Return to beginning
    inFile.clear();
    inFile.seekg(0,ios::beg);

    return id;
}


