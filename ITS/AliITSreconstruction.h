#ifndef ALIITSRECONSTRUCTION_H
#define ALIITSRECONSTRUCTION_H
/* Copyright (c) 1998-2001, ALICE Experiment at CERN, All rights reserved *
 * See cxx source for full Copyright notice                               */
 
/*
  $Id$
 */

#include <TTask.h>

class TString;
class AliITS;

class AliITSreconstruction : public TTask{
 public:
    AliITSreconstruction(); // default constructor
    AliITSreconstruction(const char *filename); // standard constructor
    virtual ~AliITSreconstruction();//Destructor
    virtual Bool_t Init();
    virtual void Exec(const Option_t *opt="ALL");
    virtual void SetOutputFile(TString filename);
 private:
    Bool_t InitRec();  // Standard Reconstrution initilization.
 private:
    TFile   *fFile;    //! pointer to the file contatining the digits and
                       // and will contain the RecPoints
    TFile   *fFile2;   //! pointer to the file that will contain RecPoints 
                       //  (set only if <>fFile)
    Bool_t  fDet[3];   //! logical specifing which detectors to reconstruct.
    Bool_t  fInit;     //! True if Init was sucessfull, else false.
    TString fFilename; //! input filename for Digits
    Int_t   fEnt;      //! Number of events to processevent index.
    Int_t   fEnt0;     //! first event to process, default 0.
    AliITS  *fITS;     //! Local pointer to ITS class.

    ClassDef(AliITSreconstruction,2) // Task to Reconstruct ITS from Digits.

};
#endif
