/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPVLukeScattering.cc
/// \brief Implementation of the G4CMPVLukeScattering class
//
// $Id$
//
// 20150111  New base class for both electron and hole Luke processes
// 20150122  Use verboseLevel instead of compiler flag for debugging

#include "G4CMPLukeScattering.hh"
#include "G4CMPConfigManager.hh"
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"
#include "G4Field.hh"
#include "G4FieldManager.hh"
#include "G4LatticeManager.hh"
#include "G4LatticePhysical.hh"
#include "G4PhononPolarization.hh"
#include "G4PhysicalConstants.hh"
#include "G4RandomDirection.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4VParticleChange.hh"
#include "Randomize.hh"
#include <iostream>
#include <fstream>


// Constructor and destructor

G4CMPLukeScattering::G4CMPLukeScattering()
  : G4CMPVDriftProcess("G4CMPLukeScattering", fLukeScattering) {
  std::ifstream LUT(G4CMPConfigManager::GetLatticeDir() + "/Ge/luke_mfp.dat");

  std::string temp;
  std::getline(LUT,temp); // First header

  if (LUT.good()) {
    LUT >> ESIZE >> EMIN >> EMAX
        >> THETASIZE >> THETAMIN >> THETAMAX
        >> MACHSIZE >> MACHMIN >> MACHMAX;
        EMIN *= volt/m;
        EMAX *= volt/m;
        THETAMIN *= rad;
        THETAMAX *= rad;
  } else {
    G4ExceptionDescription desc("Couldn't find and/or open luke_mfp.dat in "
                                + G4CMPConfigManager::GetLatticeDir());
    G4Exception("G4CMPLukeScattering::Constructor()",
                "G4CMPLuke001", FatalException, desc);
  }

/*
  G4cout << ESIZE << G4endl;
  G4cout << EMIN << G4endl;
  G4cout << EMAX << G4endl;
  G4cout << THETASIZE << G4endl;
  G4cout << THETAMIN << G4endl;
  G4cout << THETAMAX << G4endl;
  G4cout << KSIZE << G4endl;
  G4cout << KMIN << G4endl;
  G4cout << KMAX << G4endl;
  */

  std::getline(LUT,temp); // Newline char
  std::getline(LUT,temp); // Second header

  G4double code, mean, std;
  G4String fitType;
  GPILData = PDFDataTensor(ESIZE,
                           PDFDataMatrix(THETASIZE,
                                         PDFDataRow(MACHSIZE,
                                                    PDFParamTuple())));
  for (size_t i=0; i<ESIZE; ++i) {
    for (size_t j=0; j<THETASIZE; ++j) {
      for (size_t k=0; k<MACHSIZE; ++k) {
        LUT >> fitType >> mean >> std;

        if (fitType == "gaus") code = 0;
        else if (fitType == "exp") code = 1;
        else if (fitType == "flat") code = 2;
        else G4cout << "BAD" << G4endl;

        GPILData[i][j][k][0] = code;
        GPILData[i][j][k][1] = mean * ns;
        GPILData[i][j][k][2] = std * ns;
      }
    }
  }

  //MeanFreeTimes = std::vector<G4double>{0.930467,0.923749,0.917032,0.910315,0.903597,0.89688,0.890163,0.883445,0.876728,0.87001,0.863293,0.856576,0.849859,0.843143,0.836428,0.829717,0.82301,0.816308,0.809612,0.802925,0.796247,0.78958,0.782925,0.776283,0.769657,0.763046,0.756453,0.749879,0.743324,0.736791,0.73028,0.723793,0.71733,0.710893,0.704482,0.6981,0.691746,0.685422,0.679128,0.672866,0.666636,0.660439,0.654276,0.648147,0.642054,0.635997,0.629977,0.623993,0.618048,0.612141,0.606273,0.600444,0.594655,0.588906,0.583199,0.577532,0.571906,0.566323,0.560782,0.555283,0.549826,0.544413,0.539043,0.533715,0.528432,0.523192,0.517996,0.512844,0.507735,0.502671,0.497651,0.492675,0.487743,0.482855,0.478011,0.473212,0.468456,0.463745,0.459078,0.454454,0.449874,0.445338,0.440846,0.436397,0.431991,0.427628,0.423308,0.419031,0.414797,0.410605,0.406456,0.402348,0.398282,0.394258,0.390275,0.386333,0.382432,0.378572,0.374752,0.370972,0.367232,0.363532,0.359871,0.356249,0.352665,0.34912,0.345614,0.342145,0.338713,0.335319,0.331962,0.328642,0.325358,0.32211,0.318897,0.315721,0.312579,0.309472,0.3064,0.303361,0.300357,0.297386,0.294448,0.291544,0.288671,0.285832,0.283024,0.280247,0.277502,0.274788,0.272105,0.269452,0.26683,0.264236,0.261673,0.259138,0.256633,0.254156,0.251707,0.249286,0.246892,0.244526,0.242187,0.239875,0.237589,0.235329,0.233095,0.230887,0.228703,0.226545,0.224412,0.222303,0.220218,0.218156,0.216119,0.214104,0.212113,0.210145,0.208199,0.206275,0.204373,0.202493,0.200634,0.198797,0.19698,0.195184,0.193409,0.191654,0.189919,0.188203,0.186507,0.18483,0.183172,0.181533,0.179913,0.178311,0.176727,0.175161,0.173612,0.172081,0.170568,0.169071,0.167591,0.166128,0.164681,0.16325,0.161836,0.160437,0.159054,0.157686,0.156333,0.154996,0.153673,0.152365,0.151071,0.149792,0.148527,0.147276,0.146039,0.144815,0.143605,0.142408,0.141224,0.140053,0.138895,0.137749,0.136616,0.135495,0.134387,0.13329,0.132206,0.131133,0.130071,0.129021,0.127982,0.126955,0.125938,0.124933,0.123938,0.122954,0.12198,0.121016,0.120063,0.11912,0.118186,0.117263,0.116349,0.115445,0.114551,0.113665,0.11279,0.111923,0.111065,0.110216,0.109376,0.108544,0.107722,0.106907,0.106101,0.105304,0.104514,0.103733,0.102959,0.102194,0.101436,0.100686,0.0999433,0.0992084,0.0984808,0.0977605,0.0970475,0.0963417,0.0956429,0.0949511,0.0942662,0.093588,0.0929167,0.0922519,0.0915938,0.0909421,0.0902968,0.0896579,0.0890253,0.0883988,0.0877784,0.0871641,0.0865558,0.0859534,0.0853567,0.0847659,0.0841807,0.0836012,0.0830272,0.0824588,0.0818957,0.081338,0.0807857,0.0802385,0.0796966,0.0791598,0.078628,0.0781012,0.0775794,0.0770625,0.0765504,0.0760431,0.0755405,0.0750426,0.0745493,0.0740605,0.0735763,0.0730965,0.0726211,0.0721501,0.0716834,0.0712209,0.0707627,0.0703086,0.0698586,0.0694127,0.0689709,0.068533,0.068099,0.067669,0.0672427,0.0668203,0.0664017,0.0659867,0.0655755,0.0651679,0.0647638,0.0643634,0.0639664,0.063573,0.063183,0.0627964,0.0624131,0.0620332,0.0616566,0.0612832,0.0609131,0.0605461,0.0601823,0.0598216,0.0594641,0.0591095,0.058758,0.0584095,0.0580639,0.0577212,0.0573815,0.0570446,0.0567105,0.0563793,0.0560508,0.055725,0.055402,0.0550816,0.054764,0.0544489,0.0541364,0.0538265,0.0535192,0.0532144,0.0529121,0.0526122,0.0523148,0.0520198,0.0517272,0.051437,0.0511491,0.0508635,0.0505803,0.0502993,0.0500205,0.049744,0.0494697,0.0491976,0.0489276,0.0486598,0.0483941,0.0481304,0.0478689,0.0476094,0.0473519,0.0470965,0.046843,0.0465916,0.046342,0.0460944,0.0458488,0.045605,0.0453631,0.045123,0.0448848,0.0446484,0.0444138,0.044181,0.04395,0.0437207,0.0434932,0.0432674,0.0430433,0.0428208,0.0426001,0.0423809,0.0421635,0.0419476,0.0417334,0.0415207,0.0413096,0.0411001,0.0408921,0.0406857,0.0404807,0.0402773,0.0400754,0.0398749,0.0396759,0.0394783,0.0392822};
  //MeanFreeTimes = std::vector<G4double>{0.930466584548,0.937183929965,0.94390127534,0.950618620836,0.957335967037,0.964053311876,0.97077065683,0.977488002844,0.98420534833,0.990922693899,0.997640039445,1.00435735889,1.01107434081,1.01779021986,1.02450387645,1.03121392313,1.03791876239,1.04461662524,1.05130560245,1.05798366236,1.0646486727,1.07129840411,1.0779305452,1.08454270871,1.09113243891,1.09769721292,1.10423444634,1.11074149874,1.11721567361,1.12365422307,1.13005434887,1.13641320535,1.14272790265,1.14899550393,1.15521303514,1.16137747953,1.16748578345,1.17353485702,1.17952157314,1.18544277628,1.19129527887,1.19707586337,1.20278128644,1.20840827867,1.21395354884,1.21941378466,1.22478565342,1.23006581168,1.23525089357,1.24033752715,1.24532232951,1.25020190923,1.25497287441,1.25963182551,1.26417536952,1.26860011394,1.2729026723,1.27707966886,1.28112773834,1.28504353315,1.28882371959,1.29246498892,1.29596405581,1.29931766415,1.30252257942,1.30557561566,1.308473614,1.31121346001,1.31379208408,1.31620645803,1.31845361426,1.32053063397,1.32243465779,1.3241629006,1.32571259386,1.32708110367,1.32826583422,1.3292642684,1.33007396462,1.33069257354,1.33111782723,1.33134750854,1.3313796821,1.33121215315,1.33084316039,1.33027088941,1.32949367165,1.32850991133,1.32731820854,1.32591717628,1.32430561117,1.32248241487,1.32044661534,1.31819736414,1.31573394901,1.31305578361,1.31016242248,1.30705355265,1.30372895155,1.30018872286,1.29643288086,1.29246167295,1.28827552746,1.2838749914,1.27926076655,1.27443369438,1.26939478329,1.26414517255,1.25868617505,1.2530192418,1.24714598001,1.24106815262,1.23478765969,1.22830657465,1.22162710679,1.21475162051,1.2076826266,1.2004227855,1.19297490461,1.1853419331,1.17752696499,1.16953323167,1.16136410597,1.15302309705,1.14451379909,1.13584011233,1.12700580366,1.11801493733,1.10887165454,1.09958020728,1.09014496938,1.08057041529,1.07086112676,1.0610217857,1.0510571643,1.0409721308,1.03077163812,1.02046071376,1.01004446693,0.999528073694,0.988916774625,0.978215869633,0.96743071105,0.956566699932,0.945629277055,0.934623921739,0.923556139052,0.912431462083,0.901255438936,0.890033629523,0.878771600264,0.867474916575,0.856149136748,0.844799806405,0.833432451266,0.82205257443,0.810665645612,0.799277098424,0.787892323564,0.776516662851,0.765155404994,0.753813774151,0.742496935239,0.731209978436,0.719957917923,0.708745687218,0.697578133223,0.686460011321,0.675395983461,0.664390602381,0.653448330201,0.642573510044,0.63177037589,0.621043044103,0.610395511412,0.599831652398,0.58935521432,0.578969816907,0.56867894111,0.5584859459,0.548394045694,0.538406316169,0.528525696725,0.518754985303,0.509096831639,0.499553749736,0.490128104396,0.480822116029,0.471637864485,0.462577275906,0.453642135738,0.444834096554,0.436154645008,0.427605140403,0.419186687103,0.410900685581,0.402747742733,0.394728762052,0.386844408141,0.379095208153,0.371481559122,0.364003729649,0.356661863395,0.349455977157,0.342385972466,0.335451630686,0.328652619108,0.321988495293,0.315458709445,0.309062599887,0.302799419326,0.296668313544,0.290668336661,0.28479845744,0.279057555477,0.27344442319,0.267957790186,0.262596300883,0.25735853378,0.252243001038,0.247248153539,0.242372384105,0.237614031935,0.23297138612,0.22844269059,0.224026145023,0.219719913808,0.215522124772,0.211430873611,0.207444232393,0.203560247586,0.199776943771,0.196092330483,0.192504403538,0.189011147504,0.185610539763,0.18230055802,0.179079168874,0.175944337637,0.172894075263,0.16992633219,0.167039114039,0.164230425025,0.161498283426,0.158840727643,0.156255805851,0.153741592698,0.151296181803,0.148917690314,0.146604260293,0.14435405991,0.142165284823,0.140036159535,0.137964938399,0.135949906612,0.13398938098,0.132081711179,0.130225280232,0.128418503747,0.126659834,0.124947756446,0.123280791689,0.121657496732,0.120076463101,0.118536318914,0.117035727235,0.115573387312,0.114148033966,0.112758437138,0.111403402291,0.110081769637,0.108792413897,0.107534244331,0.10630620297,0.105107268141,0.10393644686,0.102792781182,0.101675344212,0.100583240319,0.0995156042885,0.0984716009273,0.0974504242599,0.0964512966363,.095,0.0945162197017,0.0935788517893,0.092660695642,0.0917611070356,0.0908794647474,0.0900151722212,0.089167655512,0.0883363630644,0.0875207643319,0.0867203502139,0.0859346312504,0.0851631338077,0.0844054177854,0.0836610391618,0.0829295857137,0.0822106585132,0.081503874892,0.0808088674592,0.0801252837525,0.0794527857199,0.0787910489687,0.0781397625021,0.0774986276355,0.0768673580389,0.0762456788506,0.0756333265609,0.0750300479491,0.0744356002053,0.0738497497569,0.0732722739654,0.0727029566469,0.0721415917042,0.0715879807395,0.0710419330853,0.0705032654634,0.0699718015854,0.0694473719102,0.0689298133799,0.0684189688681,0.0679146871673,0.0674168226238,0.0669252348466,0.0664397884847,0.065960353024,0.0654868023702,0.0650190150596,0.0645568736512,0.0641002646519,0.0636490783001,0.0632032085521,0.062762552763,0.0623270115156,0.0618964886121,0.0614708908092,0.0610501277052,0.0606341122784,0.060222757483,0.0598159827074,0.0594137071021,0.0590158527488,0.0586223439004,0.058233106896,0.0578480700709,0.0574671636733,0.0570903197926,0.0567174722802,0.0563485566746,0.0559835101349,0.0556222713793,0.0552647806283,0.0549109795455,0.0545608111812,0.0542142200001,0.0538711515222,0.0535315527263,0.053195371731,0.0528625578112,0.0525330613477,0.052206833848,0.0518838277882,0.0515639966849,0.0512472950308,0.0509336782645,0.0506231027223,0.050315525687,0.0500109052381,0.049709200312,0.0494103706555,0.0491143768026,0.0488211800514,0.0485307424434,0.0482430267424,0.0479579964151,0.0476756156114,0.0473958491464,0.047118662483,0.0468440217144,0.0465718935485,0.0463022452922,0.0460350448363,0.0457702606415,0.0455078617237,0.0452478176413,0.0449900984816,0.0447346748486,0.0444815178509,0.0442305990895,0.0439818906471,0.0437353650766,0.0434909953908,0.0432487550517,0.0430086179609,0.0427705584496,0.0425345512695,0.0423005715833,0.0420685949563,0.0418385973475};

#ifdef G4CMP_DEBUG
  output.open("LukePhononEnergies");
#endif
}

G4CMPLukeScattering::~G4CMPLukeScattering() {
#ifdef G4CMP_DEBUG
  output.close();
#endif
}

// Physics

G4double
G4CMPLukeScattering::PostStepGetPhysicalInteractionLength(
                        const G4Track& aTrack,
                        G4double, G4ForceCondition* condition) {
  *condition = NotForced;

  G4double velocity = GetVelocity(aTrack);
  //FIXME: This should be a global wave vector, not local
  G4ThreeVector k0 = GetLocalWaveVector(aTrack);
  G4double kSound = CalculateKSound(aTrack);

  if (verboseLevel > 1) {
    G4cout << "LukeScattering v = " << velocity/m*s << " kmag = " << k0.mag()*m
	   << G4endl;
  }

  G4FieldManager* fMan =
    aTrack.GetVolume()->GetLogicalVolume()->GetFieldManager();

  G4ThreeVector E0(0., 0., 0.);
  G4double theta;
  size_t thetabin = 0;
  if (fMan && fMan->DoesFieldExist()) {
    const G4Field* field = fMan->GetDetectorField();
    //FIXME: This should be global position?
    G4ThreeVector x0 = GetLocalPosition(aTrack);
    G4double fieldValue[6];
    field->GetFieldValue(&x0[0], fieldValue);

    E0 = G4ThreeVector(fieldValue[3], fieldValue[4], fieldValue[5]);
    theta = acos((k0*E0)/k0.mag()/E0.mag());
    if (theta > THETAMAX) {
      thetabin = THETASIZE-1;
    } else {
      thetabin = THETAMAX && THETASIZE ? round(theta-THETAMIN)/((THETAMAX-THETAMIN)/(THETASIZE-1)) : 0;
    }
  }

  if (k0.mag() <= MACHMIN + (MACHMAX-MACHMIN)/(MACHSIZE-1) &&
      E0.mag() <= EMIN + (EMAX-EMIN)/(ESIZE-1)) {
    if (verboseLevel > 1) {
      G4cout << "LukeScattering PIL = DBL_MAX" << G4endl;
    }
    return DBL_MAX;
  }

  size_t machbin;
  if (k0.mag()/kSound > MACHMAX) {
    machbin = MACHSIZE-1;
  } else {
    machbin = MACHMAX && MACHSIZE ? round(k0.mag()/kSound-MACHMIN)/((MACHMAX-MACHMIN)/(MACHSIZE-1)) : 0;
  }

  size_t ebin;
  if (E0.mag() > EMAX) {
    ebin = ESIZE-1;
  } else {
    ebin = EMAX && ESIZE ? round(E0.mag()-EMIN)/((EMAX-EMIN)/(ESIZE-1)) : 0;
  }

  if (thetabin >= THETASIZE || ebin >= ESIZE || machbin >= MACHSIZE) {
    // exception
    G4cout << "Bad" << G4endl;
    G4cout << thetabin << G4endl;
    G4cout << THETASIZE << G4endl;
    G4cout << ebin << G4endl;
    G4cout << ESIZE << G4endl;
    G4cout << machbin << G4endl;
    G4cout << MACHSIZE << G4endl;
  }
  /*
    G4cout << k0.mag()/kSound << G4endl;
    G4cout << kbin << G4endl;
    G4cout << E0.mag()/volt*m << G4endl;
    G4cout << ebin << G4endl;
    G4cout << acos(k0*E0/k0.mag()/E0.mag()) << G4endl;
    G4cout << thetabin << G4endl;
    */

  G4double code = GPILData[ebin][thetabin][machbin][0];
  G4double mean = GPILData[ebin][thetabin][machbin][1];
  G4double std = GPILData[ebin][thetabin][machbin][2];

/*
  // There is a minimum pil that makes physical sense when k0 < kSound
  G4double pilMin;
  if (theta > 0.5 * pi) {
    pilMin = 0.5 * hbar_Planck * hbar_Planck /
             aTrack.GetDynamicParticle()->GetMass() / eplus / E0.mag() /
             cos(theta) * (-k0.mag2()) +
             0.5 * hbar_Planck * hbar_Planck /
             aTrack.GetDynamicParticle()->GetMass() / eplus / E0.mag() /
             cos(pi - theta) * (kSound*kSound);
  } else if (theta == 0.5 * pi) {
    pilMin = 0.5 * hbar_Planck * hbar_Planck /
             aTrack.GetDynamicParticle()->GetMass() / eplus / E0.mag() /
             cos(theta) * (kSound*kSound - k0.mag2());
  } else {
    pilMin = 0.5 * hbar_Planck * hbar_Planck /
             aTrack.GetDynamicParticle()->GetMass() / eplus / E0.mag() /
             cos(theta) * (kSound*kSound - k0.mag2());
  }

  pilMin = pilMin > 0 ? pilMin : 0;

  G4double pil = -DBL_MAX;
*/
  G4double pilMin = 0;
  G4double pil = 0;
  G4double dt = 0;
  if (code == 0) {
    G4int count = 0;
    while (pil <= pilMin) {
      dt = G4RandGauss::shoot(mean, std);
      pil = c_squared / aTrack.GetDynamicParticle()->GetMass() *
            (hbar_Planck * k0 * dt + 0.5 * eplus * E0 * dt * dt).mag();
      if (++count >= 1000) {
        G4cout << "Gauss" << G4endl;
        G4cout << pil << G4endl;
        G4cout << pilMin << G4endl;
        G4cout << dt << G4endl;
        G4cout << E0/volt*m << G4endl;
        G4cout << E0.mag()/volt*m << G4endl;
        G4cout << k0/kSound << G4endl;
        G4cout << k0.mag()/kSound << G4endl;
        G4cout << acos((k0*E0)/k0.mag()/E0.mag())/degree << G4endl;
        G4cout << (k0*E0)/k0.mag()/E0.mag() << G4endl;
        G4cout << ebin << G4endl;
        G4cout << machbin << G4endl;
        G4cout << thetabin << G4endl;
        G4cout << code << G4endl;
        G4cout << mean/ns << G4endl;
        G4cout << std/ns << G4endl;
        std::cin.get();
      }
    }
  } else if (code == 1) {
    G4int count = 0;
    while (pil <= pilMin) {
      dt = G4RandExponential::shoot(mean);
      pil = c_squared / aTrack.GetDynamicParticle()->GetMass() *
            (hbar_Planck * k0 * dt + 0.5 * eplus * E0 * dt * dt).mag();
      if (++count >= 1000) {
        G4cout << "Exp" << G4endl;
        G4cout << pil << G4endl;
        G4cout << pilMin << G4endl;
        G4cout << E0.mag()/volt*m << G4endl;
        G4cout << k0.mag()/kSound << G4endl;
        G4cout << acos((k0*E0)/k0.mag()/E0.mag())/degree << G4endl;
        G4cout << ebin << G4endl;
        G4cout << machbin << G4endl;
        G4cout << thetabin << G4endl;
        G4cout << code << G4endl;
        G4cout << mean/um << G4endl;
        G4cout << std/um << G4endl;
        std::cin.get();
      }
    }
  } else if (code == 2) {
    if (verboseLevel > 1) {
      G4cout << "LukeScattering PIL = DBL_MAX" << G4endl;
    }
    return DBL_MAX;
  } else {
    G4cout << E0.mag()/volt*m << G4endl;
    G4cout << k0.mag()/kSound << G4endl;
    G4cout << code << G4endl;
    G4ExceptionDescription
        desc("LukeScattering lookup table invalid code for GPIL");
    G4Exception("G4CMPLukeScattering::GetPhysicalInteractionLength",
                "G4CMPLuke002", EventMustBeAborted, desc);
  }

  //if (verboseLevel > 1) {
  if (pil < 0) {
    G4cout << "LukeScattering PIL = " << pil << G4endl;
    G4cout << pilMin << G4endl;
    G4cout << (k0*E0)/k0.mag()/E0.mag() << G4endl;
    G4cout << k0.mag()*m << G4endl;
    G4cout << thetabin << G4endl;
    G4cout << ebin << G4endl;
    G4cout << machbin << G4endl;
    G4cout << k0.mag()/ kSound << G4endl;
  }

  return pil;
}


G4VParticleChange* G4CMPLukeScattering::PostStepDoIt(const G4Track& aTrack,
                                                     const G4Step& aStep) {
  aParticleChange.Initialize(aTrack); 
  G4StepPoint* postStepPoint = aStep.GetPostStepPoint();
  
  G4ThreeVector ktrk = GetLocalWaveVector(aTrack);
  if (GetCurrentParticle() == G4CMPDriftElectron::Definition()) {
    ktrk = theLattice->MapPtoK_HV(GetValleyIndex(aTrack),
				  GetLocalMomentum(aTrack));
  }
  G4double kmag = ktrk.mag();
  G4double kSound = CalculateKSound(aTrack);

  // GPIL returns a length that is too short for very few cases because
  // the PDF in the "Gaussian" case is not truly Gaussian at the low tail.
  if (kmag <= kSound) {
    return &aParticleChange;
  }

  if (verboseLevel > 1) {
    G4cout << "p (post-step) = " << postStepPoint->GetMomentum()
	   << "\np_mag = " << postStepPoint->GetMomentum().mag()
	   << "\nktrk = " << ktrk
     << "\nkmag = " << kmag << " k/ks = " << kmag/kSound
     << "\nacos(ks/k) = " << acos(kSound/kmag) << G4endl;
  }

  G4double theta_phonon = MakePhononTheta(kmag, kSound);
  G4double phi_phonon   = G4UniformRand()*twopi;
  G4double q = 2*(kmag*cos(theta_phonon)-kSound);

  // Sanity check for phonon production: should be forward, like Cherenkov
  if (theta_phonon>acos(kSound/kmag) || theta_phonon>halfpi) {
    G4cerr << GetProcessName() << " ERROR: Phonon production theta_phonon "
     << theta_phonon << " exceeds cone angle " << acos(kSound/kmag)
	   << G4endl;
    return &aParticleChange;
  }
  
  // Generate phonon momentum vector
  G4ThreeVector kdir = ktrk.unit();
  G4ThreeVector qvec = q*kdir;
  qvec.rotate(kdir.orthogonal(), theta_phonon);
  qvec.rotate(kdir, phi_phonon);

  G4double Ephonon = MakePhononEnergy(kmag, kSound, theta_phonon);
#ifdef G4CMP_DEBUG
  output << Ephonon/eV << G4endl;
#endif

  // Get recoil wavevector, convert to new momentum
  G4ThreeVector k_recoil = ktrk - qvec;

  if (verboseLevel > 1) {
    G4cout << "theta_phonon = " << theta_phonon
	   << " phi_phonon = " << phi_phonon
	   << "\nq = " << q << "\nqvec = " << qvec << "\nEphonon = " << Ephonon
	   << "\nk_recoil = " << k_recoil
	   << "\nk_recoil-mag = " << k_recoil.mag()
	   << G4endl;
  }

  // Create real phonon to be propagated, with random polarization
  static const G4double genLuke = G4CMPConfigManager::GetGenPhonons();
  if (genLuke > 0. && G4UniformRand() < genLuke) {
    MakeGlobalPhononK(qvec);  		// Convert phonon vector to real space

    G4Track* phonon = CreatePhonon(G4PhononPolarization::UNKNOWN,qvec,Ephonon);
    aParticleChange.SetNumberOfSecondaries(1);
    aParticleChange.AddSecondary(phonon);
  }

  MakeGlobalRecoil(k_recoil);		// Converts wavevector to momentum
  FillParticleChange(GetValleyIndex(aTrack), k_recoil);

  aParticleChange.ProposeNonIonizingEnergyDeposit(Ephonon);
  ResetNumberOfInteractionLengthLeft();
  return &aParticleChange;
}

G4double
G4CMPLukeScattering::CalculateKSound(const G4Track& track) {
  G4double mass = track.GetDynamicParticle()->GetMass()/c_squared;
  G4double vSound = theLattice->GetSoundSpeed();
  return vSound*mass/hbar_Planck;
}
