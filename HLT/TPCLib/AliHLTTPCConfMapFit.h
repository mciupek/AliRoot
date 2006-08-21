// @(#) $Id$
// Original: AliL3ConfMapFit.h,v 1.5 2004/07/05 09:03:11 loizides 

#ifndef ALIHLTTPC_ConfMapFit
#define ALIHLTTPC_ConfMapFit

class AliHLTTPCConfMapTrack;
class AliHLTTPCVertex;

class AliHLTTPCConfMapFit {

 private:
  AliHLTTPCConfMapTrack *fTrack; //!
  AliHLTTPCVertex *fVertex; //!
  
 public:
  AliHLTTPCConfMapFit (AliHLTTPCConfMapTrack *track,AliHLTTPCVertex *vertex);
  virtual ~AliHLTTPCConfMapFit() {};

  Int_t FitHelix();
  Int_t FitCircle();
  Int_t FitLine();

  ClassDef(AliHLTTPCConfMapFit,1) //Conformal mapping fit class
};

#endif
