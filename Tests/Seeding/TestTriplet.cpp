//Acts Core
#include "Acts/Seeding/New_Seedmaker.hpp"
#include "Acts/Seeding/BinFinder.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/InternalSeed.hpp"
#include "Acts/Seeding/InternalSpacePoint.hpp"
//Acts Tests
#include "SpacePoint.hpp"
//std lib
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <string>


std::vector<std::vector<const SpacePoint*> > readFile(std::string filename){

  std::string line;
  std::vector<std::vector<const SpacePoint*> > readSP;

  std::ifstream spFile(filename);
  if (spFile.is_open()){
    while(!spFile.eof())
    {
      std::getline(spFile, line);
      if (line == "")
      {
        break;
      }
      std::stringstream ss(line);
      std::string coordStr;
      std::vector<const SpacePoint*> triplet;
      std::vector<float> threeSP;
      while(std::getline(ss,coordStr,',')){
        std::istringstream os(coordStr);
        float coord;
        os >> coord;
        threeSP.push_back(coord);
      }
      for (int i = 0; i<3; i++){
        float x = threeSP[i*3+0];
        float y = threeSP[i*3+1];
        float z = threeSP[i*3+2];
        float r = std::sqrt(x*x+y*y);
        SpacePoint * sp = new SpacePoint{x,y,z,r,0,0,0};
        triplet.push_back(sp);
      }
      readSP.push_back(triplet);
      triplet.clear();
    }
  }
  return readSP;
}


Acts::SeedmakerConfig createConfig(std::string configFile){
  Acts::SeedmakerConfig config;
  float rMax = 600;
  float rMin = 25;

  // derived from SP
  config.rMax = rMax;
  config.deltaRMax = rMax-rMin;
  config.zMin = -2800.;
  config.zMax = 2800.;

  config.deltaRMin = 5.;
  config.minPt = 500.;
  config.bFieldInZ = 0.00199724;
  config.beamPos={0.,0.};
  config.impactMax=10.;
  config.collisionRegionMin = -250.;
  config.collisionRegionMax = 250.;
  config.maxSeedsPerSpM = 5;
  //2.7 eta
  config.cotThetaMax = 7.40627;
  config.sigmaScattering = 5.00000;
  Acts::SeedFilterConfig sfconf;

  std::ifstream configStream(configFile);


  std::string line;
  if (configStream.is_open()){
    while(!configStream.eof()){
      std::getline(configStream, line);
      std::istringstream is_line(line);
      std::string key;
      if( std::getline(is_line, key, '=') )
      {
        std::string value;
        if( std::getline(is_line, value) ){ 
          if(key == "rMax"){
            config.rMax = std::stof(value);
          }
          if(key == "deltaRMax"){
            config.deltaRMax = std::stof(value);
          }
          if(key == "zMin"){
            config.zMin = std::stof(value);
          }
          if(key == "zMax"){
            config.zMax = std::stof(value);
          }
          if(key == "deltaRMin"){
            config.deltaRMin = std::stof(value);
          }
          if(key == "minPt"){
            config.minPt = std::stof(value);
          }
          if(key == "bFieldInZ"){
            config.bFieldInZ = std::stof(value);
          }
          if(key == "beamPosX"){
            config.beamPos[0] = std::stof(value);
          }
          if(key == "beamPosY"){
            config.beamPos[1] = std::stof(value);
          }
          if(key == "impactMax"){
            config.impactMax = std::stof(value);
          }
          if(key == "collisionRegionMin"){
            config.collisionRegionMin = std::stof(value);
          }
          if(key == "collisionRegionMax"){
            config.collisionRegionMax = std::stof(value);
          }
          if(key == "maxSeedsPerSpM"){
            config.maxSeedsPerSpM = std::stoi(value);
          }
          if(key == "cotThetaMax"){
            config.cotThetaMax = std::stof(value);
          }
          if(key == "sigmaScattering"){
            config.sigmaScattering = std::stof(value);
          }
        }
      }
    }
  }
  std::cout << "maxSeedsPerSpM:" << std::endl;
  std::cout << config.maxSeedsPerSpM << std::endl;

  config.seedFilter = std::make_unique<Acts::SeedFilter>(Acts::SeedFilter(sfconf));
  return config;
}

int main(int argc,  char** argv){
  if (argc !=4){
    std::cout<< "call program as: ./testTriplets config.txt triplet.cvs outfile.cvs" << std::endl;
    return -1;
  }
  std::vector<std::vector<const SpacePoint*> > vectorOfTriplets = readFile(argv[2]);

  auto bottomBinFinder = std::make_shared<Acts::BinFinder>(Acts::BinFinder());
  auto topBinFinder = std::make_shared<Acts::BinFinder>(Acts::BinFinder());
  // covariance tool, sets covariances per spacepoint as required
  std::function<Acts::Vector2D(const SpacePoint*,float,float,float)> ct = []
    (const SpacePoint*,float,float,float s=1)
    -> Acts::Vector2D
    {
      return {0,0};
    }; 

  std::ofstream outFile(argv[3]);
  if (!outFile.is_open()){
    std::cout << "couldn't create output file " << argv[3] << std::endl;
    return -1;
  }
  auto config = createConfig(argv[1]);
  Acts::New_Seedmaker sm(config);
  for(auto& triplet : vectorOfTriplets){
    std::shared_ptr<Acts::SeedmakerState> state = sm.initState<SpacePoint>(triplet, ct, bottomBinFinder, topBinFinder); 
    Acts::SeedingStateIterator end = state->end(); 

    for(Acts::SeedingStateIterator it = state->begin(); !(it == end); ++it){ 
      sm.createSeedsForSP(it, state); 
    }
    for (int i=0; i< triplet.size()-1; i++){
      auto sp = triplet[i];
      outFile << sp->x()<< ","<< sp->y()<< ","<< sp->z()<< ","; 
    }
    outFile << triplet[2]->x() << "," << triplet[2]->y() << "," << triplet[2]->z() << "," << state->outputQueue.size()<<"\n";
  }
}
