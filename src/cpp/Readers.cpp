#include "Readers.h"

/////////////
// Readers //
/////////////

void HeaderReader::setContext(InputStream* _inputStreamPtr, Timestep* _timestepPtr, int* lineNumPtr) {
  inputStreamPtr = _inputStreamPtr;
  timestepPtr = _timestepPtr;
  lineNumPtr = lineNumPtr;
}

void HeaderReader::readHeader() {
  string junk;
  inputStreamPtr->stream >> junk >> timestepPtr->time;
  inputStreamPtr->stream.ignore();
  *lineNumPtr++;

  cout << endl;
  cout << "Timestep " << timestepPtr->time << " @ line " << *lineNumPtr-1 << endl;
}


void LineReader::setContext(InputStream* _inputStreamPtr, int* _atomNumPtr, int* _lineNumPtr) {
  inputStreamPtr = _inputStreamPtr;
  atomNumPtr = _atomNumPtr;
  lineNumPtr = _lineNumPtr;
}

//Split a string into a string vector of words
vector<string> LineReader::strSplit(string str)
{
  int len=str.length();
  stringstream ss(str);
  int numWords=1;
  bool foundSpace=false;

  //Count number of words
  for(int ch=0;ch<len;ch++)
    {
      if(isspace(str[ch]))
        foundSpace=true;
      if(!isspace(str[ch])&&foundSpace)
        {
          numWords++;
          foundSpace=false;
        }
    }

  //Allocate array
  vector<string> arr(numWords);

  //Read string into array
  for(int i=0;i<len;i++)
    ss >> arr[i];

  return arr;
}

//Get index and coordinates from string array of words
void LineReader::strToData(double* coords,double* velocities,double& dipole,string line)
{
  string str;

  int index;
  //Split string into array of words
  vector<string> strArr=strSplit(line);

  //Get index
  str=strArr[0];
  index=atoi(str.data());

  //Save values
  //string -> cstring -> double
  for(int i=0;i<3;i++)
    {
      //Get coordinates from the second, third, and fourth elements in the array
      str=strArr[1+i];
      *coords++=atof(str.data());

      //Get velocities from the sixth, seventh, and eighth elements in the array
      str=strArr[5+i];
      *velocities++=atof(str.data());
    }

  //Save dipole moment cos(theta)
  dipole=atof(strArr[10].data());
}

void LineReader::readLine() {
  double coords[3], velocities[3];
  double dipole;
  string line;

  getline(inputStreamPtr->stream,line);
  *lineNumPtr++;

  //Save data
  strToData(coords,velocities,dipole,line);
  atom.x = coords[0];
  atom.y = coords[1];
  atom.z = coords[2];

  atom.vx = velocities[0];
  atom.vy = velocities[1];
  atom.vz = velocities[2];

  atom.cosTheta = dipole;
  *atomNumPtr++;

  // READ THE LINE AND STORE DATA IN atom
}

void TimestepReader::setContext(InputStream* _inputStreamPtr, AtomArray* atomArrayPtr, Timestep* _timestepPtr, SimData* _simDataPtr) {
  inputStreamPtr = _inputStreamPtr;
  lineReader.setContext(inputStreamPtr, &atomNum, &lineNum);
  headerReader.setContext(inputStreamPtr, timestepPtr, &lineNum);
  timestepPtr = _timestepPtr;
  simDataPtr = _simDataPtr;
}

void TimestepReader::resetAtomCounter() {
  atomNum = 0;
}

void TimestepReader::readTimestep() {
  resetAtomCounter();

  headerReader.readHeader();
  cout << "Step # " << timestepPtr->time << endl;

  for(int i=0; i<atomArrayPtr->numAtoms; i++) {
    lineReader.readLine();
    atomArrayPtr->setAtom(i, lineReader.atom);
  }

  timestepPtr->stepNum++;
}

void FrameReader::openStream(Options options) {
  inputStream.open(options.dumpfile);
}

void FrameReader::setContext(Options options, AtomArray* _atomArrayPtr, SimData* _simDataPtr) {
  atomArrayPtr = _atomArrayPtr;
  simDataPtr = _simDataPtr;

  stepsPerFrame = simDataPtr->stepsPerFrame;
  openStream(options);

  timestepReader.setContext(&inputStream, atomArrayPtr, timestepPtr, simDataPtr);
}

FrameReader::FrameReader(Options options, AtomArray* _atomArrayPtr, SimData* _simDataPtr) {
  setContext(options, _atomArrayPtr, _simDataPtr);
}

void FrameReader::updateFrame() {
  frame.frameStep = timestepPtr->stepNum;
  frame.time = timestepPtr->time;
}

void FrameReader::readFrame() {
  // Read first timestep separately to set frame variables
  // to those of the first timestep in the frame.
  timestepReader.readTimestep();
  updateFrame();

  // Then read the rest
  for(int n; n<stepsPerFrame-1; n++) {
    timestepReader.readTimestep();
  }
}

void InitialTimestepReader::openStream(Options options) {
  inputStream.open(initLoc);
}

void InitialTimestepReader::setContext(Options options, AtomArray* _atomArrayPtr, SimData* _simDataPtr) {
  atomArrayPtr = _atomArrayPtr;
  simDataPtr = _simDataPtr;

  initLoc = "../init.txt";

  openStream(options);

  timestepReader.setContext(&inputStream, atomArrayPtr, &emptyTimestep, simDataPtr);
}

InitialTimestepReader::InitialTimestepReader(Options options, AtomArray* _atomArrayPtr, SimData* _simDataPtr) {
  setContext(options, _atomArrayPtr, _simDataPtr);
}

void InitialTimestepReader::readFromFile() {
  for(int i=0; i<simDataPtr->numAtoms; i++) {
    inputStream.stream >> atom.x >> atom.y >> atom.z;
    atomArrayPtr->setAtom(i, atom);
  }
}

void InitialTimestepReader::readFromStream() {
  // Read first timestep from input file
  // (same file other timesteps will be read from)
  // This will be called if the initial timestep file
  // doesn't exist.
  timestepReader.readTimestep();
}

bool InitialTimestepReader::fileExists() {
  // Return true if file exists, false otherwise.
  ifstream testStream(initLoc);
  return testStream.is_open();
}

void InitialTimestepReader::writeToFile() {
  FILE* outFile = fopen(initLoc.data(), "w");

  for(int i=0; i<simDataPtr->numAtoms; i++) {
    atomArrayPtr->getAtom(i, atom);
    fprintf(outFile, "%10.5f %10.5f %10.5f", atom.x, atom.y, atom.z);
    fflush(outFile);
  }
}

void InitialTimestepReader::readInitialTimestep() {
  // Read first timestep if the file `initLoc` doesn't exist,
  // then write to that file.
  // If it does exist, read from the file
  if(fileExists()) {
    readFromFile();
  }

  else {
    readFromStream();
    writeToFile();
  }
}

