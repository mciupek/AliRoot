#ifndef AliMFTCATrack_H
#define AliMFTCATrack_H

#include "TObject.h"
#include "TClonesArray.h"

#include "AliMFTConstants.h"

class AliMFTCACell;

//_________________________________________________________________________________
class AliMFTCATrack : public TObject {
  
public:
  
  AliMFTCATrack();
  ~AliMFTCATrack() {};
  
  // copy constructor
  AliMFTCATrack (const AliMFTCATrack &track);
  
  virtual void Clear(Option_t *);
  void AddCell(AliMFTCACell *cell);
  void SetGID(Int_t gid) { fGID = gid; }
  const Int_t GetGID() const { return fGID; }
  const Int_t GetNcells() const { return fNcells; }
  const Int_t GetCellGID(Int_t ic) const { return fCellGIDarray[ic]; }
  AliMFTCACell* GetCell(Int_t ic) const { return (AliMFTCACell*)fCells->At(ic); }
  const Int_t GetLastCellGID() const { return fCellGIDarray[fNcells-1]; }
  const Int_t GetStartLayer() const { return fStartLayer; }
  void SetStartLayer(Int_t sl) { fStartLayer = sl; }
  void SetMCflag(UChar_t mcf) { fMCflag = mcf; }
  const UChar_t GetMCflag() { return fMCflag; }
  void SetXatDCA(Double_t xdca) { fXatDCA = xdca; }
  void SetYatDCA(Double_t ydca) { fYatDCA = ydca; }
  void SetZDCA(Double_t zdca) { fZDCA = zdca; }
  void SetTheta(Double_t the) { fTheta = the; }
  void SetPhi(Double_t phi) { fPhi = phi; }
  const Double_t GetXatDCA() { return fXatDCA; }
  const Double_t GetYatDCA() { return fYatDCA; }
  const Double_t GetZDCA()   { return fZDCA; }
  const Double_t GetTheta() { return fTheta; }
  const Double_t GetPhi() { return fPhi; }
  void SetChiSqX(Double_t chisq) { fChiSqX = chisq; }
  void SetChiSqY(Double_t chisq) { fChiSqY = chisq; }
  const Double_t GetChiSqX() { return fChiSqX; }
  const Double_t GetChiSqY() { return fChiSqY; }
  Double_t AddCellToChiSq(AliMFTCACell *cell);
  void SetMCindex(Int_t index) { fMCindex = index; }
  const Int_t GetMCindex() { return fMCindex; }
  
private:
  static const Int_t fNDetMax = AliMFTConstants::fNMaxPlanes;

  Int_t  fGID;                    // Track global identifier
  Int_t  fNcells;                 // Number of cells in the track
  Int_t  fStartLayer;             // Upstream start layer (RunBackward)
  Int_t  fCellGIDarray[fNDetMax]; // Array of cell global identifier
  
  TClonesArray *fCells;           //! Array of cells
  
  UChar_t fMCflag;                // MC classification of the track:
                                  // 0 = default (not set)
                                  // 1 = clean
                                  // 2 = good
                                  // 3 = fake
                                  // 4 = noise
  
  Double_t fZDCA;                 // z of the DCA between the track
                                  // and the beam axis [cm]
  Double_t fXatDCA;               // x at zDCA [cm]
  Double_t fYatDCA;               // y at zDCA [cm]
  Double_t fTheta;                // theta fit [deg]
  Double_t fPhi;                  // phi fit [deg]
  Double_t fChiSqX;               // reduced ChiSq en xz
  Double_t fChiSqY;               // reduced ChiSq en yz
  Int_t    fMCindex;              // MC track index for clean tracks
  
  ClassDef(AliMFTCATrack,1);
  
};

#endif

