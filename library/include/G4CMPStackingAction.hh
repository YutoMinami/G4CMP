//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file library/include/G4CMPStackingAction.hh
/// \brief Definition of the G4CMPStackingAction class
//
// $Id$
//
#ifndef G4CMPStackingAction_h
#define G4CMPStackingAction_h 1

#include "globals.hh"
#include "G4UserStackingAction.hh"
#include "G4CMPProcessUtils.hh"

class G4Track;

class G4CMPStackingAction
  : public G4UserStackingAction, public G4CMPProcessUtils {
public:
  G4CMPStackingAction();
  virtual ~G4CMPStackingAction();

public:
  virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* aTrack);

protected:
  void SetPhononWaveVector(const G4Track* theTrack) const;
  void SetPhononVelocity(const G4Track* theTrack) const;

  void SetChargeCarrierValley(const G4Track* theTrack) const;
  void SetChargeCarrierMass(const G4Track* theTrack) const;
};

#endif /* G4CMPStackingAction_h */
