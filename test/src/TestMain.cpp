#include "Parameters.h"
#include "MDBase.h"
#include "Atoms.h"
#include "Readers.h"
#include "Writers.h"
#include "Droplet.h"
#include "Substrate.h"
#include "Visualization.h"
#include "catch.hpp"

using namespace std;

const int NUM_ATOMS = 513699;
const int NUM_STEPS = 8;
const int NUM_WATER = 1105;
const int STEPS_PER_FRAME = 3;
const double SUBSTRATE_MASS = 3130904.0;
const double DROPLET_MASS = 6563.56701333;

TEST_CASE("Readers", "[lotus]") {
  int argc = 2;
  const char* argv[2] = {"test", "../test/data/test_config.yaml"};
  CommandLineParser commandLineParser(argc, argv);
  Options options = commandLineParser.options;
  REQUIRE(strcmp(options.configPath, argv[1]) == 0);
  REQUIRE(options.liquidTypes[0] == 4);
  REQUIRE(options.liquidTypes[1] == 5);
  REQUIRE(options.solidTypes[0] == 1);
  REQUIRE(options.solidTypes[1] == 2);
  REQUIRE(options.solidTypes[2] == 3);
  REQUIRE(options.geometry == "spherical");
  REQUIRE(options.stepsPerFrame == STEPS_PER_FRAME);
  REQUIRE(options.dumpfile == "../test/data/20A_atom1_13-20");
  REQUIRE(options.datafile == "../test/data/lammps_noZperiod_3A.dat");
  REQUIRE(options.outLoc == "../test/results");
  REQUIRE(options.skipToEnd == false);
  REQUIRE(options.trackMonoAtoms == false);
  REQUIRE(options.saveImages == true);
  REQUIRE(options.plotHist == false);
  REQUIRE(options.plotDipole == false);
  REQUIRE(options.plotVr == false);
  REQUIRE(options.plotDensity == false);
  REQUIRE(options.plotAllTogether == false);
  // REQUIRE(options.verbose == false);
  REQUIRE(options.onlyFindInterface == false);
  SimData simData(options);
  DatafileReader datafileReader(simData);
  REQUIRE(simData.masses.size() == 5);
  REQUIRE(simData.masses[1] == 26.981540);
  REQUIRE(simData.masses[2] == 15.994915);
  REQUIRE(simData.masses[3] == 1.007825);
  REQUIRE(simData.masses[4] == 15.9994);
  REQUIRE(simData.masses[5] == 1.00794);
  REQUIRE(simData.numAtoms == NUM_ATOMS);
  REQUIRE(simData.waterBonds.size() == NUM_WATER);

  AtomArray atoms(simData);
  REQUIRE(atoms.numAtoms == NUM_ATOMS);

  DumpfileReader dumpfileReader(atoms);
  REQUIRE(simData.lastFrame.frameNum == 1);
  REQUIRE(simData.lastFrame.numSteps == 5);
  REQUIRE(simData.numSteps == NUM_STEPS);

  Substrate substrate(atoms, 5);
  Droplet droplet(atoms);

  DensFigure densFigure("dens", droplet, substrate);
  DropletFigure dropletFigure("droplet", droplet);
  TanhFigure tanhFigure("tanh", droplet.monolayer.tanhFit);
  ScalarWriter scalarWriter("results.txt", dumpfileReader, droplet);

  // Time loop
  while(dumpfileReader.good()) {
    dumpfileReader.readFrame();
    substrate.fill(atoms);
    // REQUIRE(abs(substrate.getMass() - SUBSTRATE_MASS) < 1e-3*SUBSTRATE_MASS);
    droplet.fill(atoms);
    // REQUIRE(abs(droplet.getMass() - DROPLET_MASS) < 1e-3*DROPLET_MASS);
    // This is just mass in cylinder, not total
    // TODO: Fill monolayer
    droplet.findMonolayer();

    // TODO: Save image
    densFigure.draw();
    densFigure.save();

    droplet.monolayer.fill(atoms);
    REQUIRE(droplet.monolayer.hMono->GetEntries() > 0);
    REQUIRE(droplet.monolayer.zlim[1] - droplet.monolayer.zlim[0] > 0);

    droplet.monolayer.calculateRadius();

    tanhFigure.draw();
    tanhFigure.save();

    REQUIRE(droplet.monolayer.radius > 0);

    droplet.dropletCalculations();

    // TODO: Draw droplet figure
    dropletFigure.draw();
    dropletFigure.save();


    REQUIRE(droplet.bulk.gCirclePoints->GetN() > 0);
    REQUIRE(droplet.bulk.circle.GetNumPoints() > 0);

    REQUIRE(droplet.bulk.circle.intersected);
    REQUIRE(droplet.bulk.height > 0);
    REQUIRE(droplet.bulk.radius > 0);
    REQUIRE(droplet.bulk.contactAngle >= 0.0);
    REQUIRE(droplet.bulk.contactAngle <= 180.0);
    // TODO: Write frame quantities to file
    //       - rm, rb, ca, h, circle points

    scalarWriter.writeFrame();

    // TODO: Add option to enable/disable circle fit

    // TODO: Clean up print statements

    // REQUIRE(file_exists("out.png"));
  }
}

