#pragma once

#include "Acts/Seeding/SeedSPGrid.hpp"
#include "Acts/Seeding/InternalSeed.hpp"
#include "Acts/Seeding/Seed.hpp"
#include "Acts/Seeding/IBinFinder.hpp"

#include <vector>
#include <memory>
#include <queue>
#include <mutex>

namespace Acts{

    class SeedingStateIterator{
    public:
      SeedingStateIterator&
      operator++(){
        if (spIndex < currentBin->size()-1){
          spIndex++;
          return *this;
        }
        if(zIndex < phiZbins[1]){
          spIndex = 0;
          zIndex++;
        }else if(phiIndex < phiZbins[0]){
          spIndex = 0;
          zIndex=1;
          phiIndex++;
        }else{
          spIndex++;
        }
        // loop over all bins starting from current phiBin (i) and zBin (j) 
        size_t j = zIndex;
        for(size_t i = phiIndex; i <= phiZbins[0]; i++){
          for(; j <= phiZbins[1]; j++){
            currentBin = &(grid->at({i,j}));
            if(currentBin->size() > 0){
              phiIndex = i;
              zIndex = j;
              bottomBinIndices = bottomBinFinder->findBins(phiIndex, zIndex, grid);
              topBinIndices = topBinFinder->findBins(phiIndex, zIndex, grid);
              return *this;
            }
          }
          j=1;
        }
        phiIndex = phiZbins[0];
        zIndex   = phiZbins[1];
        spIndex  = 0;
        bottomBinIndices = bottomBinFinder->findBins(phiIndex, zIndex, grid);
        topBinIndices = topBinFinder->findBins(phiIndex, zIndex, grid);
        return *this; 
      }

      bool
      operator==(const SeedingStateIterator& otherState){
        return (spIndex == otherState.spIndex && zIndex == otherState.zIndex && phiIndex == otherState.phiIndex);
      }

      SeedingStateIterator(const SPGrid* spgrid, IBinFinder* botBinFinder, IBinFinder* tBinFinder):
                                                                 currentBin(&(spgrid->at({1,1}))){
        grid = spgrid;
        bottomBinFinder = botBinFinder;
        topBinFinder = tBinFinder;
        phiZbins = grid->getNBins();
        bottomBinIndices = bottomBinFinder->findBins(phiIndex, zIndex, grid);
        topBinIndices = topBinFinder->findBins(phiIndex, zIndex, grid);
        for(size_t i = 1; i <= phiZbins[0]; i++){
          for(size_t j = 1; j <= phiZbins[1]; j++){
            if ( currentBin->size()==0){
              currentBin = &(spgrid->at({i,j}));
              phiIndex = i;
              zIndex = j;
              bottomBinIndices = bottomBinFinder->findBins(phiIndex, zIndex, grid);
              topBinIndices = topBinFinder->findBins(phiIndex, zIndex, grid);
            }
          }
        }
      }
      
      SeedingStateIterator(const SPGrid* spgrid, IBinFinder* botBinFinder, IBinFinder* tBinFinder, size_t phiInd, size_t zInd, size_t spInd):
                                                                 currentBin(&(spgrid->at({phiInd,zInd}))){
        bottomBinFinder = botBinFinder;
        topBinFinder = tBinFinder;
        grid = spgrid;
        phiIndex = phiInd;
        zIndex = zInd;
        spIndex = spInd;
        phiZbins = grid->getNBins();
        bottomBinIndices = bottomBinFinder->findBins(phiIndex, zIndex, grid);
        topBinIndices = topBinFinder->findBins(phiIndex, zIndex, grid);
      }

      // middle spacepoint bin
      const std::vector<std::unique_ptr<const InternalSpacePoint > > * currentBin;
      std::set<size_t> bottomBinIndices;
      std::set<size_t> topBinIndices;
      const SPGrid* grid;
      size_t phiIndex = 1;
      size_t zIndex = 1;
      size_t spIndex =0;
      std::array<long unsigned int,2ul> phiZbins;
      IBinFinder* bottomBinFinder;
      IBinFinder* topBinFinder;
    };
     
    struct SeedmakerState {
      // grid with ownership of all InternalSpacePoint
      std::unique_ptr<Acts::SPGrid> binnedSP;

      // BinFinder must return std::vector<Acts::Seeding::Bin> with content of 
      // each bin sorted in r (ascending)
      std::shared_ptr<IBinFinder> bottomBinFinder;
      std::shared_ptr<IBinFinder> topBinFinder;

      // container with seeds created so far
      std::queue<std::unique_ptr<const InternalSeed> > outputQueue;
      // mutex to protect output access
      std::mutex outputMutex;

      SeedingStateIterator
      begin(){
        return SeedingStateIterator(binnedSP.get(), bottomBinFinder.get(), topBinFinder.get());
      }

      SeedingStateIterator
      end(){
        auto phiZbins = binnedSP->getNBins();
        return SeedingStateIterator(binnedSP.get(), bottomBinFinder.get(), topBinFinder.get(), phiZbins[0], phiZbins[1], binnedSP->at(phiZbins).size());
      }

    };

}
