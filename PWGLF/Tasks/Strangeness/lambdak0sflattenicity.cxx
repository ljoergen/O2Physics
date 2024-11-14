// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
///
/// Making modifications to the Strangeness Tutorial code
/// The code is still in development mode
/// Flattenicity part of the code is adopted from
/// https://github.com/AliceO2Group/O2Physics/blob/master/PWGMM/Mult/Tasks/flatenicityFV0.cxx
/// For any suggestions, commets or questions, Please write to Suraj Prasad
/// (Suraj.Prasad@cern.ch)

#include <cmath>
// #include <tuple>
#include <vector>

// #include "Framework/ASoAHelpers.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/AnalysisTask.h"
#include "Framework/HistogramRegistry.h"
// #include "Framework/StaticFor.h"
#include "Framework/runDataProcessing.h"

// #include "Common/Core/TrackSelection.h"
// #include "Common/Core/TrackSelectionDefaults.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/Multiplicity.h"
#include "Common/DataModel/TrackSelectionTables.h"
// #include "ReconstructionDataFormats/Track.h"
#include "Framework/O2DatabasePDGPlugin.h"

#include <TGraph.h>

#include "Common/DataModel/PIDResponse.h"
#include "PWGLF/DataModel/LFStrangenessTables.h"
#include "PWGLF/Utils/inelGt.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

struct lambdak0sflattenicity {
  // Histograms are defined with HistogramRegistry
  Service<o2::framework::O2DatabasePDG> pdg;
  HistogramRegistry rEventSelection{"eventSelection",
                                    {},
                                    OutputObjHandlingPolicy::AnalysisObject,
                                    true,
                                    true};
  HistogramRegistry rKzeroShort{
    "kzeroShort",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};
  HistogramRegistry rLambda{
    "lambda",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};
  HistogramRegistry rAntiLambda{
    "antilambda",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};
  HistogramRegistry rFlattenicity{
    "flattenicity",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};

  static constexpr std::string_view nhEst[8] = {
    "eGlobaltrack", "eFV0", "e1flatencityFV0", "eFT0",
    "e1flatencityFT0", "eFV0FT0C", "e1flatencityFV0FT0C", "ePtTrig"};
  static constexpr std::string_view tEst[8] = {
    "GlobalTrk", "FV0", "1-flatencity_FV0", "FT0",
    "1-flatencityFT0", "FV0_FT0C", "1-flatencity_FV0_FT0C", "PtTrig"};
  static constexpr std::string_view nhPtEst[8] = {
    "ptVsGlobaltrack", "ptVsFV0",
    "ptVs1flatencityFV0", "ptVsFT0",
    "ptVs1flatencityFT0", "ptVsFV0FT0C",
    "ptVs1flatencityFV0FT0C", "pTVsPtTrig"};

  // Configurable for histograms
  Configurable<int> nBinsVz{"nBinsVz", 100, "N bins in Vz"};
  Configurable<int> nBinsK0sMass{"nBinsK0sMass", 200, "N bins in K0sMass"};
  Configurable<int> nBinsLambdaMass{"nBinsLambdaMass", 200,
                                    "N bins in LambdaMass"};
  Configurable<int> nBinspT{"nBinspT", 250, "N bins in pT"};
  Configurable<int> nBinsFlattenicity{"nBinsFlattenicity", 100, "N bins in Flattenicity"};

  // Configurable for event selection

  Configurable<bool> applyEvSel{"applyEvSel", true,
                                "Apply event selection to Data and MCRec"};
  Configurable<bool> Issel8{"Issel8", true,
                            "Accept events that pass sel8 selection"};
  Configurable<float> cutzvertex{"cutzvertex", 10.0f,
                                 "Accepted z-vertex range (cm)"};
  Configurable<bool> IsINELgt0{"isINELgt0", true, "is INEL gt 0"};
  Configurable<bool> IsNoTimeFrameBorder{
    "IsNoTimeFrameBorder", true,
    "cut branch crossing at the beginning/end of TF"};
  Configurable<bool> IsNoITSROFrameBorder{
    "IsNoITSROFrameBorder", true,
    "cut branch crossing at the beginning/end of ITS ROF"};
  Configurable<bool> IsVertexITSTPC{"IsVertexITSTPC", false,
                                    "Is Vertex ITSTPC"};
  Configurable<bool> IsNoSameBunchPileup{"IsNoSameBunchPileup", false,
                                         "Is No Same Bunch Pileup"};
  Configurable<bool> IsGoodZvtxFT0vsPV{"IsGoodZvtxFT0vsPV", false,
                                       "Is Good Zvtx FT0 vs PV"};
  Configurable<bool> IsTriggerTVX{"IsTriggerTVX", true,
                                  "coincidence of a signal in FT0A and FT0C"};

  // Configurables for Flattenicity
  Configurable<bool> flattenicityQA{"flattenicityQA", true, "Store Flattenicity QA plots"};
  Configurable<bool> applyCalibCh{"applyCalibCh", false, "equalize FV0"};
  Configurable<bool> applyCalibVtx{"applyCalibVtx", false,
                                   "equalize FV0 vs vtx"};
  Configurable<bool> applyNorm{"applyNorm", false, "normalization to eta"};
  Configurable<bool> isflattenicitywithFV0{"isflattenicitywithFV0", true,
                                           "Calculate Flattenicity with FV0"};
  Configurable<bool> isflattenicitywithFT0{"isflattenicitywithFT0", true,
                                           "Calculate Flattenicity with FT0"};
  Configurable<bool> isflattenicitywithFV0FT0C{
    "isflattenicitywithFV0FT0C", true,
    "Calculate Flattenicity with FV0+FT0C"};

  Configurable<int> flattenicityforanalysis{
    "flattenicityforanalysis", 0,
    "Which Flattenicity to be used for analysis, 0 for FV0, 1 for FT0, 2 for FV0+FT0C"};

  // Common Configurable parameters for V0 selection
  Configurable<float> v0setting_dcav0dau{"v0setting_dcav0dau", 1,
                                         "DCA V0 Daughters"};
  Configurable<float> v0setting_dcapostopv{"v0setting_dcapostopv", 0.06,
                                           "DCA Pos To PV"};
  Configurable<float> v0setting_dcanegtopv{"v0setting_dcanegtopv", 0.06,
                                           "DCA Neg To PV"};
  Configurable<float> v0setting_rapidity{"v0setting_rapidity", 0.5,
                                         "V0 rapidity cut"};

  // Configurable parameters for V0 selection for KOs
  Configurable<double> v0setting_cospaK0s{"v0setting_cospa_K0s", 0.97,
                                          "V0 CosPA for K0s"};
  Configurable<float> v0setting_radiusK0s{"v0setting_radius_K0s", 0.5,
                                          "v0radius for K0s"};
  Configurable<float> v0setting_ctauK0s{"v0setting_ctau_K0s", 20,
                                        "v0ctau for K0s"};
  Configurable<float> v0setting_massrejectionK0s{
    "v0setting_massrejection_K0s", 0.005,
    "Competing Mass Rejection cut for K0s"};

  // Configurable parameters for V0 selection for Lambda
  Configurable<double> v0setting_cospaLambda{"v0setting_cospa_Lambda", 0.995,
                                             "V0 CosPA for Lambda"};
  Configurable<float> v0setting_radiusLambda{"v0setting_radius_Lambda", 0.5,
                                             "v0radius for Lambda"};
  Configurable<float> v0setting_ctauLambda{"v0setting_ctau_Lambda", 30,
                                           "v0ctau for Lambda"};
  Configurable<float> v0setting_massrejectionLambda{
    "v0setting_massrejection_Lambda", 0.01,
    "Competing Mass Rejection cut for Lambda"};

  // Configurable parameters for PID selection
  Configurable<float> NSigmaTPCPion{"NSigmaTPCPion", 5, "NSigmaTPCPion"};
  Configurable<float> NSigmaTPCProton{"NSigmaTPCProton", 5, "NSigmaTPCProton"};

  // Configurable<float> v0daughter_etacut{"V0DaughterEtaCut", 0.8,
  // "V0DaughterEtaCut"};
  Configurable<float> v0_etacut{"v0EtaCut", 0.8, "v0EtaCut"};

  // acceptance cuts for Flattenicity correlation
  Configurable<float> cfgTrkEtaCut{"cfgTrkEtaCut", 0.8f,
                                   "Eta range for tracks"};
  Configurable<float> cfgTrkLowPtCut{"cfgTrkLowPtCut", 0.0f, "Minimum  pT"};

  void init(InitContext const&)
  {
    // Axes
    AxisSpec K0sMassAxis = {nBinsK0sMass, 0.45f, 0.55f,
                            "#it{M}_{#pi^{+}#pi^{-}} [GeV/#it{c}^{2}]"};
    AxisSpec LambdaMassAxis = {nBinsLambdaMass, 1.015f, 1.215f,
                               "#it{M}_{p#pi^{-}} [GeV/#it{c}^{2}]"};
    AxisSpec AntiLambdaMassAxis = {nBinsLambdaMass, 1.015f, 1.215f,
                                   "#it{M}_{#pi^{+}#bar{p}} [GeV/#it{c}^{2}]"};
    AxisSpec vertexZAxis = {nBinsVz, -15., 15., "vrtx_{Z} [cm]"};
    AxisSpec ptAxis = {nBinspT, 0.0f, 25.0f, "#it{p}_{T} (GeV/#it{c})"};
    AxisSpec flatAxis = {nBinsFlattenicity, 0.0f, 1.0f, "1-#rho_{ch}"};

    int nBinsEst[8] = {100, 500, 102, 500, 102, 500, 102, 150};
    float lowEdgeEst[8] = {-0.5, -0.5, -0.01, -0.5, -0.01, -0.5, -0.01, .0};
    float upEdgeEst[8] = {99.5, 49999.5, 1.01, 499.5, 1.01, 499.5, 1.01, 150.0};

    // Histograms
    // Event selection
    rEventSelection.add("hVertexZ", "hVertexZ",
                        {HistType::kTH1F, {vertexZAxis}});
    rEventSelection.add("hEventsSelected", "hEventsSelected",
                        {HistType::kTH1I, {{12, 0, 12}}});

    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(1, "all");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(2, "sel8");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(3, "zvertex");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(4, "TFBorder");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(5, "ITSROFBorder");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(6, "VertexITSTPC");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(7, "SameBunchPileup");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(8, "isGoodZvtxFT0vsPV");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(9, "TVX");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(10, "INEL>0");
    rEventSelection.get<TH1>(HIST("hEventsSelected"))->GetXaxis()->SetBinLabel(11, "Applied selection");

    rEventSelection.add("hFlattenicityDistribution", "hFlattenicityDistribution",
                        {HistType::kTH1F, {flatAxis}});
    if (doprocessRecMC) {
      rEventSelection.add("hFlattenicityDistributionMCGen_Rec", "hFlattenicityDistributionMCGen_Rec",
                          {HistType::kTH1F, {flatAxis}});
      rEventSelection.add("hFlattenicity_Corr_Gen_vs_Rec", "hFlattenicity_Corr_Gen_vs_Rec",
                          {HistType::kTH2F, {flatAxis, flatAxis}});
    }
    if (doprocessGenMC) {
      rEventSelection.add("hVertexZGen", "hVertexZGen",
                          {HistType::kTH1F, {vertexZAxis}});

      rEventSelection.add("hFlattenicityDistributionMCGen", "hFlattenicityDistributionMCGen",
                          {HistType::kTH1F, {flatAxis}});

      rEventSelection.add("hFlat_RecoColl_MC", "hFlat_RecoColl_MC", {HistType::kTH1F, {flatAxis}});
      rEventSelection.add("hFlat_RecoColl_MC_INELgt0", "hFlat_RecoColl_MC_INELgt0", {HistType::kTH1F, {flatAxis}});
      rEventSelection.add("hFlat_GenRecoColl_MC", "hFlat_GenRecoColl_MC", {HistType::kTH1F, {flatAxis}});
      rEventSelection.add("hFlat_GenRecoColl_MC_INELgt0", "hFlat_GenRecoColl_MC_INELgt0", {HistType::kTH1F, {flatAxis}});
      rEventSelection.add("hFlat_GenColl_MC", "hFlat_GenColl_MC", {HistType::kTH1F, {flatAxis}});
      rEventSelection.add("hFlat_GenColl_MC_INELgt0", "hFlat_GenColl_MC_INELgt0", {HistType::kTH1F, {flatAxis}});
      rEventSelection.add("hNEventsMCGen", "hNEventsMCGen", {HistType::kTH1I, {{4, 0.f, 4.f}}});
      rEventSelection.get<TH1>(HIST("hNEventsMCGen"))->GetXaxis()->SetBinLabel(1, "all");
      rEventSelection.get<TH1>(HIST("hNEventsMCGen"))->GetXaxis()->SetBinLabel(2, "zvertex_true");
      rEventSelection.get<TH1>(HIST("hNEventsMCGen"))->GetXaxis()->SetBinLabel(3, "INELgt0_true");
      rEventSelection.add("hNEventsMCGenReco", "hNEventsMCGenReco", {HistType::kTH1I, {{2, 0.f, 2.f}}});
      rEventSelection.get<TH1>(HIST("hNEventsMCGenReco"))->GetXaxis()->SetBinLabel(1, "INEL");
      rEventSelection.get<TH1>(HIST("hNEventsMCGenReco"))->GetXaxis()->SetBinLabel(2, "INELgt0");
      rEventSelection.add("hNEventsMCReco", "hNEventsMCReco", {HistType::kTH1I, {{4, 0.f, 4.f}}});
      rEventSelection.get<TH1>(HIST("hNEventsMCReco"))->GetXaxis()->SetBinLabel(1, "all");
      rEventSelection.get<TH1>(HIST("hNEventsMCReco"))->GetXaxis()->SetBinLabel(2, "pass ev sel");
      rEventSelection.get<TH1>(HIST("hNEventsMCReco"))->GetXaxis()->SetBinLabel(3, "INELgt0");
      rEventSelection.get<TH1>(HIST("hNEventsMCReco"))->GetXaxis()->SetBinLabel(4, "check");
    }
    // K0s reconstruction
    // Mass
    rKzeroShort.add("hMassK0s", "hMassK0s", {HistType::kTH1F, {K0sMassAxis}});
    rKzeroShort.add("hMassK0sSelected", "hMassK0sSelected",
                    {HistType::kTH1F, {K0sMassAxis}});

    // K0s topological/PID cuts
    rKzeroShort.add("hrapidityK0s", "hrapidityK0s",
                    {HistType::kTH1F, {{40, -2.0f, 2.0f, "y"}}});
    rKzeroShort.add("hctauK0s", "hctauK0s",
                    {HistType::kTH1F, {{40, 0.0f, 40.0f, "c#tau (cm)"}}});
    rKzeroShort.add(
      "h2DdecayRadiusK0s", "h2DdecayRadiusK0s",
      {HistType::kTH1F, {{100, 0.0f, 1.0f, "Decay Radius (cm)"}}});
    rKzeroShort.add("hDCAV0DaughtersK0s", "hDCAV0DaughtersK0s",
                    {HistType::kTH1F, {{55, 0.0f, 2.2f, "DCA Daughters"}}});
    rKzeroShort.add("hV0CosPAK0s", "hV0CosPAK0s",
                    {HistType::kTH1F, {{100, 0.95f, 1.f, "CosPA"}}});
    rKzeroShort.add("hNSigmaPosPionFromK0s", "hNSigmaPosPionFromK0s",
                    {HistType::kTH2F, {{100, -5.f, 5.f}, {ptAxis}}});
    rKzeroShort.add("hNSigmaNegPionFromK0s", "hNSigmaNegPionFromK0s",
                    {HistType::kTH2F, {{100, -5.f, 5.f}, {ptAxis}}});
    rKzeroShort.add("hMassK0spT", "hMassK0spT",
                    {HistType::kTH2F, {{K0sMassAxis}, {ptAxis}}});
    rKzeroShort.add("hMassK0spTFlat", "hMassK0spTFlat",
                    {HistType::kTH3F, {{K0sMassAxis}, {ptAxis}, {flatAxis}}});
    if (doprocessRecMC) {
      rKzeroShort.add("Generated_MCRecoCollCheck_INEL_K0Short", "Generated_MCRecoCollCheck_INEL_K0Short",
                      {HistType::kTH2F, {{ptAxis}, {flatAxis}}});
    }

    if (doprocessGenMC) {
      rKzeroShort.add("pGen_MCGenRecoColl_INEL_K0Short", "pGen_MCGenRecoColl_INEL_K0Short",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rKzeroShort.add("Generated_MCRecoColl_INEL_K0Short", "Generated_MCRecoColl_INEL_K0Short",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rKzeroShort.add("pGen_MCGenColl_INEL_K0Short", "pGen_MCGenColl_INEL_K0Short",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rKzeroShort.add("pGen_MCGenRecoColl_INELgt0_K0Short", "pGen_MCGenRecoColl_INELgt0_K0Short",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rKzeroShort.add("Generated_MCRecoColl_INELgt0_K0Short", "Generated_MCRecoColl_INELgt0_K0Short",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rKzeroShort.add("Generated_MCRecoCollCheck_INELgt0_K0Short", "Generated_MCRecoCollCheck_INELgt0_K0Short",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rKzeroShort.add("pGen_MCGenColl_INELgt0_K0Short", "pGen_MCGenColl_INELgt0_K0Short",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
    }
    // Lambda reconstruction Mass
    rLambda.add("hMassLambda", "hMassLambda",
                {HistType::kTH1F, {LambdaMassAxis}});
    rLambda.add("hMassLambdaSelected", "hMassLambdaSelected",
                {HistType::kTH1F, {LambdaMassAxis}});

    // Lambda topological/PID cuts
    rLambda.add("hDCAV0DaughtersLambda", "hDCAV0DaughtersLambda",
                {HistType::kTH1F, {{55, 0.0f, 2.2f, "DCA Daughters"}}});
    rLambda.add("hV0CosPALambda", "hV0CosPALambda",
                {HistType::kTH1F, {{100, 0.95f, 1.f, "CosPA"}}});
    rLambda.add("hNSigmaPosPionFromLambda", "hNSigmaPosPionFromLambda",
                {HistType::kTH2F, {{100, -5.f, 5.f}, {ptAxis}}});
    rLambda.add("hNSigmaNegPionFromLambda", "hNSigmaNegPionFromLambda",
                {HistType::kTH2F, {{100, -5.f, 5.f}, {ptAxis}}});
    rLambda.add("hrapidityLambda", "hrapidityLambda",
                {HistType::kTH1F, {{40, -2.0f, 2.0f, "y"}}});
    rLambda.add("hctauLambda", "hctauLambda",
                {HistType::kTH1F, {{40, 0.0f, 40.0f, "c#tau (cm)"}}});
    rLambda.add("h2DdecayRadiusLambda", "h2DdecayRadiusLambda",
                {HistType::kTH1F, {{100, 0.0f, 1.0f, "c#tau (cm)"}}});
    rLambda.add("hMassLambdapT", "hMassLambdapT",
                {HistType::kTH2F, {{LambdaMassAxis}, {ptAxis}}});
    rLambda.add("hMassLambdapTFlat", "hMassLambdapTFlat",
                {HistType::kTH3F, {{LambdaMassAxis}, {ptAxis}, {flatAxis}}});
    if (doprocessRecMC) {
      rLambda.add("Generated_MCRecoCollCheck_INEL_Lambda", "Generated_MCRecoCollCheck_INEL_Lambda",
                  {HistType::kTH2F, {{ptAxis}, {flatAxis}}});
    }

    if (doprocessGenMC) {
      rLambda.add("pGen_MCGenRecoColl_INEL_Lambda", "pGen_MCGenRecoColl_INEL_Lambda",
                  {HistType::kTH2F, {ptAxis, flatAxis}});
      rLambda.add("Generated_MCRecoColl_INEL_Lambda", "Generated_MCRecoColl_INEL_Lambda",
                  {HistType::kTH2F, {ptAxis, flatAxis}});
      rLambda.add("pGen_MCGenColl_INEL_Lambda", "pGen_MCGenColl_INEL_Lambda",
                  {HistType::kTH2F, {ptAxis, flatAxis}});
      rLambda.add("pGen_MCGenRecoColl_INELgt0_Lambda", "pGen_MCGenRecoColl_INELgt0_Lambda",
                  {HistType::kTH2F, {ptAxis, flatAxis}});
      rLambda.add("Generated_MCRecoColl_INELgt0_Lambda", "Generated_MCRecoColl_INELgt0_Lambda",
                  {HistType::kTH2F, {ptAxis, flatAxis}});
      rLambda.add("Generated_MCRecoCollCheck_INELgt0_Lambda", "Generated_MCRecoCollCheck_INELgt0_Lambda",
                  {HistType::kTH2F, {ptAxis, flatAxis}});
      rLambda.add("pGen_MCGenColl_INELgt0_Lambda", "pGen_MCGenColl_INELgt0_Lambda",
                  {HistType::kTH2F, {ptAxis, flatAxis}});
    }
    // AntiLambda reconstruction
    // Mass
    rAntiLambda.add("hMassAntiLambda", "hMassAntiLambda",
                    {HistType::kTH1F, {AntiLambdaMassAxis}});
    rAntiLambda.add("hMassAntiLambdaSelected", "hMassAntiLambdaSelected",
                    {HistType::kTH1F, {AntiLambdaMassAxis}});

    // AntiLambda topological/PID cuts
    rAntiLambda.add("hDCAV0DaughtersAntiLambda", "hDCAV0DaughtersAntiLambda",
                    {HistType::kTH1F, {{55, 0.0f, 2.2f, "DCA Daughters"}}});
    rAntiLambda.add("hV0CosPAAntiLambda", "hV0CosPAAntiLambda",
                    {HistType::kTH1F, {{100, 0.95f, 1.f, "CosPA"}}});
    rAntiLambda.add("hNSigmaPosPionFromAntiLambda",
                    "hNSigmaPosPionFromAntiLambda",
                    {HistType::kTH2F, {{100, -5.f, 5.f}, {ptAxis}}});
    rAntiLambda.add("hNSigmaNegPionFromAntiLambda",
                    "hNSigmaNegPionFromAntiLambda",
                    {HistType::kTH2F, {{100, -5.f, 5.f}, {ptAxis}}});
    rAntiLambda.add("hrapidityAntiLambda", "hrapidityAntiLambda",
                    {HistType::kTH1F, {{40, -2.0f, 2.0f, "y"}}});
    rAntiLambda.add("hctauAntiLambda", "hctauAntiLambda",
                    {HistType::kTH1F, {{40, 0.0f, 40.0f, "c#tau (cm)"}}});
    rAntiLambda.add("h2DdecayRadiusAntiLambda", "h2DdecayRadiusAntiLambda",
                    {HistType::kTH1F, {{100, 0.0f, 1.0f, "c#tau (cm)"}}});
    rAntiLambda.add("hMassAntiLambdapT", "hMassAntiLambdapT",
                    {HistType::kTH2F, {{AntiLambdaMassAxis}, {ptAxis}}});
    rAntiLambda.add("hMassAntiLambdapTFlat", "hMassAntiLambdapTFlat",
                    {HistType::kTH3F, {{AntiLambdaMassAxis}, {ptAxis}, {flatAxis}}});
    if (doprocessRecMC) {
      rAntiLambda.add("Generated_MCRecoCollCheck_INEL_AntiLambda", "Generated_MCRecoCollCheck_INEL_AntiLambda",
                      {HistType::kTH2F, {{ptAxis}, {flatAxis}}});
    }

    if (doprocessGenMC) {
      rAntiLambda.add("pGen_MCGenRecoColl_INEL_AntiLambda", "pGen_MCGenRecoColl_INEL_AntiLambda",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rAntiLambda.add("Generated_MCRecoColl_INEL_AntiLambda", "Generated_MCRecoColl_INEL_AntiLambda",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rAntiLambda.add("pGen_MCGenColl_INEL_AntiLambda", "pGen_MCGenColl_INEL_AntiLambda",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rAntiLambda.add("pGen_MCGenRecoColl_INELgt0_AntiLambda", "pGen_MCGenRecoColl_INELgt0_AntiLambda",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rAntiLambda.add("Generated_MCRecoColl_INELgt0_AntiLambda", "Generated_MCRecoColl_INELgt0_AntiLambda",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rAntiLambda.add("Generated_MCRecoCollCheck_INELgt0_AntiLambda", "Generated_MCRecoCollCheck_INELgt0_AntiLambda",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
      rAntiLambda.add("pGen_MCGenColl_INELgt0_AntiLambda", "pGen_MCGenColl_INELgt0_AntiLambda",
                      {HistType::kTH2F, {ptAxis, flatAxis}});
    }

    if (flattenicityQA) {
      rFlattenicity.add("hEv", "Ev", HistType::kTH1F,
                        {{6, -0.5, 5.5, "index activated detector"}});
      rFlattenicity.add("hFV0amplRing1to4", "FV01to4", HistType::kTH1F,
                        {{4000, -0.5, +49999.5, "FV0 amplitude"}});
      rFlattenicity.add("hFT0Aampl", "FTAampl", HistType::kTH1F,
                        {{50000, -0.5, +199999.5, "FT0A amplitude"}});
      rFlattenicity.add("hFT0Campl", "FTCampl", HistType::kTH1F,
                        {{10000, -0.5, +4999.5, "FT0C amplitude"}});
      rFlattenicity.add("hFT0C", "FT0C", HistType::kTH1F,
                        {{50000, -0.5, 199999.5, "FT0C amplitudes"}});
      rFlattenicity.add("hFT0A", "FT0A", HistType::kTH1F,
                        {{2000, -0.5, 1999.5, "FT0A amplitudes"}});

      // estimators
      for (int i_e = 0; i_e < 8; ++i_e) {
        rFlattenicity.add(
          nhEst[i_e].data(), "", HistType::kTH2F,
          {{nBinsEst[i_e], lowEdgeEst[i_e], upEdgeEst[i_e], tEst[i_e].data()},
           {100, -0.5, +99.5, "Global track"}});
      }

      // vs pT
      for (int i_e = 0; i_e < 8; ++i_e) {
        rFlattenicity.add(
          nhPtEst[i_e].data(), "", HistType::kTProfile,
          {{nBinsEst[i_e], lowEdgeEst[i_e], upEdgeEst[i_e], tEst[i_e].data()}});
      }

      rFlattenicity.add("fMultFv0", "FV0 amp", HistType::kTH1F,
                        {{5000, -0.5, +199999.5, "FV0 amplitude"}});
      rFlattenicity.add(
        "hAmpV0VsCh", "", HistType::kTH2F,
        {{48, -0.5, 47.5, "channel"}, {500, -0.5, +19999.5, "FV0 amplitude"}});
      rFlattenicity.add(
        "hAmpV0VsChBeforeCalibration", "", HistType::kTH2F,
        {{48, -0.5, 47.5, "channel"}, {500, -0.5, +19999.5, "FV0 amplitude"}});

      rFlattenicity.add(
        "hAmpT0AVsChBeforeCalibration", "", HistType::kTH2F,
        {{24, -0.5, 23.5, "channel"}, {600, -0.5, +5999.5, "FT0A amplitude"}});
      rFlattenicity.add(
        "hAmpT0CVsChBeforeCalibration", "", HistType::kTH2F,
        {{28, -0.5, 27.5, "channel"}, {600, -0.5, +5999.5, "FT0C amplitude"}});

      rFlattenicity.add(
        "hAmpT0AVsCh", "", HistType::kTH2F,
        {{24, -0.5, 23.5, "channel"}, {600, -0.5, +5999.5, "FT0A amplitude"}});
      rFlattenicity.add(
        "hAmpT0CVsCh", "", HistType::kTH2F,
        {{28, -0.5, 27.5, "channel"}, {600, -0.5, +5999.5, "FT0C amplitude"}});

      rFlattenicity.add("hFlatFT0CvsFlatFT0A", "", HistType::kTH2F,
                        {{20, -0.01, +1.01, "flatenicity (FT0C)"},
                         {20, -0.01, +1.01, "flatenicity (FT0A)"}});
      rFlattenicity.add(
        "fEtaPhiFv0", "eta vs phi", HistType::kTH2F,
        {{8, 0.0, 2 * M_PI, "#phi (rad)"}, {5, 2.2, 5.1, "#eta"}});

      rFlattenicity.add("hAmpV0vsVtxBeforeCalibration", "", HistType::kTH2F,
                        {{30, -15.0, +15.0, "Trk mult"},
                         {1000, -0.5, +39999.5, "FV0 amplitude"}});
      rFlattenicity.add(
        "hAmpT0AvsVtxBeforeCalibration", "", HistType::kTH2F,
        {{30, -15.0, +15.0, "Vtx_z"}, {600, -0.5, +5999.5, "FT0A amplitude"}});
      rFlattenicity.add(
        "hAmpT0CvsVtxBeforeCalibration", "", HistType::kTH2F,
        {{30, -15.0, +15.0, "Vtx_z"}, {600, -0.5, +5999.5, "FT0C amplitude"}});

      rFlattenicity.add("hAmpV0vsVtx", "", HistType::kTH2F,
                        {{30, -15.0, +15.0, "Trk mult"},
                         {1000, -0.5, +39999.5, "FV0 amplitude"}});
      rFlattenicity.add(
        "hAmpT0AvsVtx", "", HistType::kTH2F,
        {{30, -15.0, +15.0, "Vtx_z"}, {600, -0.5, +5999.5, "FT0A amplitude"}});
      rFlattenicity.add(
        "hAmpT0CvsVtx", "", HistType::kTH2F,
        {{30, -15.0, +15.0, "Vtx_z"}, {600, -0.5, +5999.5, "FT0C amplitude"}});
    }
    if (doprocessDataRun3 && (doprocessRecMC || doprocessGenMC)) {
      LOGF(fatal,
           "Both Data and MC are both set to true; try again with only "
           "one of them set to true");
    }
    if (!doprocessDataRun3 && !(doprocessRecMC || doprocessGenMC)) {
      LOGF(fatal,
           "Both Data and MC set to false; try again with only one of "
           "them set to false");
    }
    if ((doprocessRecMC && !doprocessGenMC) ||
        (!doprocessRecMC && doprocessGenMC)) {
      LOGF(fatal,
           "MCRec and MCGen are set to opposite switches, try again "
           "with both set to either true or false");
    }
  }

  int getT0ASector(int i_ch)
  {
    int i_sec_t0a = -1;
    for (int i_sec = 0; i_sec < 24; ++i_sec) {
      if (i_ch >= 4 * i_sec && i_ch <= 3 + 4 * i_sec) {
        i_sec_t0a = i_sec;
        break;
      }
    }
    return i_sec_t0a;
  }

  int getT0CSector(int i_ch)
  {
    int i_sec_t0c = -1;
    for (int i_sec = 0; i_sec < 28; ++i_sec) {
      if (i_ch >= 4 * i_sec && i_ch <= 3 + 4 * i_sec) {
        i_sec_t0c = i_sec;
        break;
      }
    }
    return i_sec_t0c;
  }

  int getFV0Ring(int i_ch)
  {
    int i_ring = -1;
    if (i_ch < 8) {
      i_ring = 0;
    } else if (i_ch >= 8 && i_ch < 16) {
      i_ring = 1;
    } else if (i_ch >= 16 && i_ch < 24) {
      i_ring = 2;
    } else if (i_ch >= 24 && i_ch < 32) {
      i_ring = 3;
    } else {
      i_ring = 4;
    }
    return i_ring;
  }

  int getFV0IndexPhi(int i_ch)
  {
    int i_ring = -1;

    if (i_ch >= 0 && i_ch < 8) {
      if (i_ch < 4) {
        i_ring = i_ch;
      } else {
        if (i_ch == 7) {
          i_ring = 4;
        } else if (i_ch == 6) {
          i_ring = 5;
        } else if (i_ch == 5) {
          i_ring = 6;
        } else if (i_ch == 4) {
          i_ring = 7;
        }
      }
    } else if (i_ch >= 8 && i_ch < 16) {
      if (i_ch < 12) {
        i_ring = i_ch;
      } else {
        if (i_ch == 15) {
          i_ring = 12;
        } else if (i_ch == 14) {
          i_ring = 13;
        } else if (i_ch == 13) {
          i_ring = 14;
        } else if (i_ch == 12) {
          i_ring = 15;
        }
      }
    } else if (i_ch >= 16 && i_ch < 24) {
      if (i_ch < 20) {
        i_ring = i_ch;
      } else {
        if (i_ch == 23) {
          i_ring = 20;
        } else if (i_ch == 22) {
          i_ring = 21;
        } else if (i_ch == 21) {
          i_ring = 22;
        } else if (i_ch == 20) {
          i_ring = 23;
        }
      }
    } else if (i_ch >= 24 && i_ch < 32) {
      if (i_ch < 28) {
        i_ring = i_ch;
      } else {
        if (i_ch == 31) {
          i_ring = 28;
        } else if (i_ch == 30) {
          i_ring = 29;
        } else if (i_ch == 29) {
          i_ring = 30;
        } else if (i_ch == 28) {
          i_ring = 31;
        }
      }
    } else if (i_ch == 32) {
      i_ring = 32;
    } else if (i_ch == 40) {
      i_ring = 33;
    } else if (i_ch == 33) {
      i_ring = 34;
    } else if (i_ch == 41) {
      i_ring = 35;
    } else if (i_ch == 34) {
      i_ring = 36;
    } else if (i_ch == 42) {
      i_ring = 37;
    } else if (i_ch == 35) {
      i_ring = 38;
    } else if (i_ch == 43) {
      i_ring = 39;
    } else if (i_ch == 47) {
      i_ring = 40;
    } else if (i_ch == 39) {
      i_ring = 41;
    } else if (i_ch == 46) {
      i_ring = 42;
    } else if (i_ch == 38) {
      i_ring = 43;
    } else if (i_ch == 45) {
      i_ring = 44;
    } else if (i_ch == 37) {
      i_ring = 45;
    } else if (i_ch == 44) {
      i_ring = 46;
    } else if (i_ch == 36) {
      i_ring = 47;
    }
    return i_ring;
  }

  float GetFlatenicity(std::span<float> signals)
  {
    int entries = signals.size();
    float flat = 9999;
    float mRho = 0;
    for (int iCell = 0; iCell < entries; ++iCell) {
      mRho += 1.0 * signals[iCell];
    }
    // average activity per cell
    mRho /= (1.0 * entries);
    // get sigma
    float sRho_tmp = 0;
    for (int iCell = 0; iCell < entries; ++iCell) {
      sRho_tmp += TMath::Power(1.0 * signals[iCell] - mRho, 2);
    }
    sRho_tmp /= (1.0 * entries * entries);
    float sRho = TMath::Sqrt(sRho_tmp);
    if (mRho > 0) {
      flat = sRho / mRho;
    }
    return flat;
  }
  float pdgmassK0s = 0.497614;
  float pdgmassLambda = 1.115683;
  // V0A signal and flatenicity calculation
  static constexpr float calib[48] = {
    1.01697, 1.122, 1.03854, 1.108, 1.11634, 1.14971, 1.19321,
    1.06866, 0.954675, 0.952695, 0.969853, 0.957557, 0.989784, 1.01549,
    1.02182, 0.976005, 1.01865, 1.06871, 1.06264, 1.02969, 1.07378,
    1.06622, 1.15057, 1.0433, 0.83654, 0.847178, 0.890027, 0.920814,
    0.888271, 1.04662, 0.8869, 0.856348, 0.863181, 0.906312, 0.902166,
    1.00122, 1.03303, 0.887866, 0.892437, 0.906278, 0.884976, 0.864251,
    0.917221, 1.10618, 1.04028, 0.893184, 0.915734, 0.892676};
  // calibration T0C
  static constexpr float calibT0C[28] = {
    0.949829, 1.05408, 1.00681, 1.00724, 0.990663, 0.973571, 0.9855,
    1.03726, 1.02526, 1.00467, 0.983008, 0.979349, 0.952352, 0.985775,
    1.013, 1.01721, 0.993948, 0.996421, 0.971871, 1.02921, 0.989641,
    1.01885, 1.01259, 0.929502, 1.03969, 1.02496, 1.01385, 1.01711};
  // calibration T0A
  static constexpr float calibT0A[24] = {
    0.86041, 1.10607, 1.17724, 0.756397, 1.14954, 1.0879,
    0.829438, 1.09014, 1.16515, 0.730077, 1.06722, 0.906344,
    0.824167, 1.14716, 1.20692, 0.755034, 1.11734, 1.00556,
    0.790522, 1.09138, 1.16225, 0.692458, 1.12428, 1.01127};
  // calibration factor MFT vs vtx
  static constexpr float biningVtxt[30] = {
    -14.5, -13.5, -12.5, -11.5, -10.5, -9.5, -8.5, -7.5, -6.5, -5.5,
    -4.5, -3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5, 4.5,
    5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5};

  // calibration factor FV0 vs vtx
  static constexpr float calibFV0vtx[30] = {
    0.907962, 0.934607, 0.938929, 0.950987, 0.950817, 0.966362,
    0.968509, 0.972741, 0.982412, 0.984872, 0.994543, 0.996003,
    0.99435, 1.00266, 0.998245, 1.00584, 1.01078, 1.01003,
    1.00726, 1.00872, 1.01726, 1.02015, 1.0193, 1.01106,
    1.02229, 1.02104, 1.03435, 1.00822, 1.01921, 1.01736};
  // calibration FT0A vs vtx
  static constexpr float calibFT0Avtx[30] = {
    0.924334, 0.950988, 0.959604, 0.965607, 0.970016, 0.979057,
    0.978384, 0.982005, 0.992825, 0.990048, 0.998588, 0.997338,
    1.00102, 1.00385, 0.99492, 1.01083, 1.00703, 1.00494,
    1.00063, 1.0013, 1.00777, 1.01238, 1.01179, 1.00577,
    1.01028, 1.017, 1.02975, 1.0085, 1.00856, 1.01662};
  // calibration FT0C vs vtx
  static constexpr float calibFT0Cvtx[30] = {
    1.02096, 1.01245, 1.02148, 1.03605, 1.03561, 1.03667,
    1.04229, 1.0327, 1.03674, 1.02764, 1.01828, 1.02331,
    1.01864, 1.015, 1.01197, 1.00615, 0.996845, 0.993051,
    0.985635, 0.982883, 0.981914, 0.964635, 0.967812, 0.95475,
    0.956687, 0.932816, 0.92773, 0.914892, 0.891724, 0.872382};

  static constexpr int nEta5 = 2; // FT0C + FT0A
  static constexpr float weigthsEta5[nEta5] = {0.0490638, 0.010958415};
  static constexpr float deltaEeta5[nEta5] = {1.1, 1.2};

  static constexpr int nEta6 = 2; // FT0C + FV0
  static constexpr float weigthsEta6[nEta6] = {0.0490638, 0.00353962};
  static constexpr float deltaEeta6[nEta6] = {1.1, 2.9};

  static constexpr int innerFV0 = 32;
  static constexpr float maxEtaFV0 = 5.1;
  static constexpr float minEtaFV0 = 2.2;
  static constexpr float detaFV0 = (maxEtaFV0 - minEtaFV0) / 5.0;

  static constexpr int nCells = 48; // 48 sectors in FV0
  std::array<float, nCells> RhoLattice;
  std::array<float, nCells> RhoLatticeFV0AMC;
  std::array<float, nCells> ampchannel;
  std::array<float, nCells> ampchannelBefore;
  static constexpr int nCellsT0A = 24;
  std::array<float, nCellsT0A> RhoLatticeT0A;
  static constexpr int nCellsT0C = 28;
  std::array<float, nCellsT0C> RhoLatticeT0C;

  std::array<float, 8> estimator;

  template <typename TCollision>
  bool isEventSelected(TCollision const& collision)
  {
    rEventSelection.fill(HIST("hEventsSelected"), 0.5);

    if (Issel8 && !collision.sel8()) {
      return false;
    }

    rEventSelection.fill(HIST("hEventsSelected"), 1.5);
    if (TMath::Abs(collision.posZ()) > cutzvertex) {
      return false;
    }

    rEventSelection.fill(HIST("hEventsSelected"), 2.5);

    if (IsNoTimeFrameBorder &&
        !collision.selection_bit(o2::aod::evsel::kNoTimeFrameBorder)) {
      return false;
    }
    rEventSelection.fill(HIST("hEventsSelected"), 3.5);

    if (IsNoITSROFrameBorder &&
        !collision.selection_bit(o2::aod::evsel::kNoITSROFrameBorder)) {
      return false;
    }
    rEventSelection.fill(HIST("hEventsSelected"), 4.5);

    if (IsVertexITSTPC &&
        !collision.selection_bit(o2::aod::evsel::kIsVertexITSTPC)) {
      return false;
    }
    rEventSelection.fill(HIST("hEventsSelected"), 5.5);

    if (IsNoSameBunchPileup &&
        !collision.selection_bit(o2::aod::evsel::kNoSameBunchPileup)) {
      return false;
    }
    rEventSelection.fill(HIST("hEventsSelected"), 6.5);

    if (IsGoodZvtxFT0vsPV &&
        !collision.selection_bit(o2::aod::evsel::kIsGoodZvtxFT0vsPV)) {
      return false;
    }
    rEventSelection.fill(HIST("hEventsSelected"), 7.5);

    if (IsTriggerTVX &&
        !collision.selection_bit(o2::aod::evsel::kIsTriggerTVX)) {
      return false;
    }
    rEventSelection.fill(HIST("hEventsSelected"), 8.5);

    if (IsINELgt0 && (collision.isInelGt0() == false)) {
      return false;
    }
    rEventSelection.fill(HIST("hEventsSelected"), 9.5);

    return true;
  }

  // ============== Flattenicity estimation begins  ===================== //
  template <typename TCollision, typename Tracks>
  float EstimateFlattenicity(TCollision const& collision, Tracks const& tracks)
  {
    const int nDetVtx = 3;
    TGraph* gVtx[nDetVtx];
    const char* nameDet[nDetVtx] = {"AmpV0", "AmpT0A", "AmpT0C"};

    float ampl5[nEta5] = {0, 0};
    float ampl6[nEta6] = {0, 0};

    for (int i_d = 0; i_d < nDetVtx; ++i_d) {
      gVtx[i_d] = 0;
      gVtx[i_d] = new TGraph();
    }
    for (int i_v = 0; i_v < 30; ++i_v) {
      gVtx[0]->SetPoint(i_v, biningVtxt[i_v], calibFV0vtx[i_v]);
    }
    for (int i_v = 0; i_v < 30; ++i_v) {
      gVtx[1]->SetPoint(i_v, biningVtxt[i_v], calibFT0Avtx[i_v]);
    }
    for (int i_v = 0; i_v < 30; ++i_v) {
      gVtx[2]->SetPoint(i_v, biningVtxt[i_v], calibFT0Cvtx[i_v]);
    }

    for (int i_d = 0; i_d < nDetVtx; ++i_d) {
      gVtx[i_d]->SetName(Form("g%s", nameDet[i_d]));
    }
    auto vtxZ = collision.posZ();

    float sumAmpFV0 = 0;
    float sumAmpFV01to4Ch = 0;

    ampchannel.fill(0.0);
    ampchannelBefore.fill(0.0);
    RhoLattice.fill(0);

    if ((isflattenicitywithFV0 || isflattenicitywithFV0FT0C) &&
        collision.has_foundFV0()) {

      auto fv0 = collision.foundFV0();
      for (std::size_t ich = 0; ich < fv0.amplitude().size(); ich++) {
        float phiv0 = -999.0;
        float etav0 = -999.0;
        int channelv0 = fv0.channel()[ich];
        float ampl_ch = fv0.amplitude()[ich];
        int ringindex = getFV0Ring(channelv0);
        int channelv0phi = getFV0IndexPhi(channelv0);
        etav0 = maxEtaFV0 - (detaFV0 / 2.0) * (2.0 * ringindex + 1);
        if (channelv0 < innerFV0) {
          phiv0 = (2.0 * (channelv0phi - 8 * ringindex) + 1) * M_PI / (8.0);
        } else {
          phiv0 = ((2.0 * channelv0phi) + 1 - 64.0) * 2.0 * M_PI / (32.0);
        }
        ampchannelBefore[channelv0phi] = ampl_ch;
        if (applyCalibCh) {
          ampl_ch *= calib[channelv0phi];
        }
        sumAmpFV0 += ampl_ch;

        if (channelv0 >= 8) { // exclude the 1st ch, eta 2.2,4.52
          sumAmpFV01to4Ch += ampl_ch;
        }
        if (flattenicityQA) {
          rFlattenicity.fill(HIST("fEtaPhiFv0"), phiv0, etav0, ampl_ch);
        }
        ampchannel[channelv0phi] = ampl_ch;
        if (channelv0 < innerFV0) {
          RhoLattice[channelv0phi] = ampl_ch;
        } else {
          RhoLattice[channelv0phi] = ampl_ch / 2.0; // two channels per bin
        }
      }

      if (flattenicityQA) {
        rFlattenicity.fill(HIST("hAmpV0vsVtxBeforeCalibration"), vtxZ, sumAmpFV0);
      }
      if (applyCalibVtx) {
        sumAmpFV0 *= gVtx[0]->Eval(vtxZ);
        sumAmpFV01to4Ch *= gVtx[0]->Eval(vtxZ);
      }
      if (flattenicityQA) {
        rFlattenicity.fill(HIST("hAmpV0vsVtx"), vtxZ, sumAmpFV0);
      }
    }

    float flattenicityfv0 = 9999;
    if (isflattenicitywithFV0 || isflattenicitywithFV0FT0C) {
      flattenicityfv0 = GetFlatenicity({RhoLattice.data(), RhoLattice.size()});
    }

    // global tracks
    float ptT = 0.;
    int multGlob = 0;
    for (auto& track : tracks) {
      if (!track.isGlobalTrack()) {
        continue;
      }
      if (track.pt() > ptT) {
        ptT = track.pt();
      }
      multGlob++;
    }

    // FT0
    float sumAmpFT0A = 0.f;
    float sumAmpFT0C = 0.f;

    RhoLatticeT0A.fill(0);
    RhoLatticeT0C.fill(0);

    if ((isflattenicitywithFT0 || isflattenicitywithFV0FT0C) &&
        collision.has_foundFT0()) {
      auto ft0 = collision.foundFT0();
      if (isflattenicitywithFT0) {
        for (std::size_t i_a = 0; i_a < ft0.amplitudeA().size(); i_a++) {
          float amplitude = ft0.amplitudeA()[i_a];
          uint8_t channel = ft0.channelA()[i_a];
          int sector = getT0ASector(channel);
          if (sector >= 0 && sector < 24) {
            RhoLatticeT0A[sector] += amplitude;
            if (flattenicityQA) {
              rFlattenicity.fill(HIST("hAmpT0AVsChBeforeCalibration"), sector,
                                 amplitude);
            }
            if (applyCalibCh) {
              amplitude *= calibT0A[sector];
            }
            if (flattenicityQA) {
              rFlattenicity.fill(HIST("hAmpT0AVsCh"), sector, amplitude);
            }
          }
          sumAmpFT0A += amplitude;
          if (flattenicityQA) {
            rFlattenicity.fill(HIST("hFT0A"), amplitude);
          }
        }
      }

      for (std::size_t i_c = 0; i_c < ft0.amplitudeC().size(); i_c++) {
        float amplitude = ft0.amplitudeC()[i_c];
        sumAmpFT0C += amplitude;
        uint8_t channel = ft0.channelC()[i_c];
        int sector = getT0CSector(channel);
        if (sector >= 0 && sector < 28) {
          RhoLatticeT0C[sector] += amplitude;
          if (flattenicityQA) {
            rFlattenicity.fill(HIST("hAmpT0CVsChBeforeCalibration"), sector,
                               amplitude);
          }
          if (applyCalibCh) {
            amplitude *= calibT0C[sector];
          }
          if (flattenicityQA) {
            rFlattenicity.fill(HIST("hAmpT0CVsCh"), sector, amplitude);
          }
        }
        if (flattenicityQA) {
          rFlattenicity.fill(HIST("hFT0C"), amplitude);
        }
      }
      if (flattenicityQA) {
        rFlattenicity.fill(HIST("hAmpT0AvsVtxBeforeCalibration"), vtxZ,
                           sumAmpFT0A);
        rFlattenicity.fill(HIST("hAmpT0CvsVtxBeforeCalibration"), vtxZ,
                           sumAmpFT0C);
      }
      if (applyCalibVtx) {
        sumAmpFT0A *= gVtx[1]->Eval(vtxZ);
        sumAmpFT0C *= gVtx[2]->Eval(vtxZ);
      }
      if (flattenicityQA) {
        rFlattenicity.fill(HIST("hAmpT0AvsVtx"), vtxZ, sumAmpFT0A);
        rFlattenicity.fill(HIST("hAmpT0CvsVtx"), vtxZ, sumAmpFT0C);
      }
    }
    float flatenicity_t0a = 9999;
    if (isflattenicitywithFT0) {
      flatenicity_t0a =
        GetFlatenicity({RhoLatticeT0A.data(), RhoLatticeT0A.size()});
    }
    float flatenicity_t0c = 9999;
    if (isflattenicitywithFT0 || isflattenicitywithFV0FT0C) {
      flatenicity_t0c =
        GetFlatenicity({RhoLatticeT0C.data(), RhoLatticeT0C.size()});
    }

    bool isOK_estimator5 = false;
    bool isOK_estimator6 = false;
    float combined_estimator5 = 0;
    float combined_estimator6 = 0;

    for (int i_e = 0; i_e < 8; ++i_e) {
      estimator[i_e] = 0;
    }

    if (collision.has_foundFV0() && collision.has_foundFT0()) {
      float all_weights = 0;
      // option 5
      ampl5[0] = sumAmpFT0C;
      ampl5[1] = sumAmpFT0A;
      if (sumAmpFT0C > 0 && sumAmpFT0A > 0) {
        isOK_estimator5 = true;
      }
      if (isOK_estimator5) {
        if (applyNorm) {
          all_weights = 0;
          for (int i_5 = 0; i_5 < nEta5; ++i_5) {
            combined_estimator5 +=
              ampl5[i_5] * weigthsEta5[i_5] / deltaEeta5[i_5];
            all_weights += weigthsEta5[i_5];
          }
          combined_estimator5 /= all_weights;
        } else {
          for (int i_5 = 0; i_5 < nEta5; ++i_5) {
            combined_estimator5 += ampl5[i_5] * weigthsEta5[i_5];
          }
        }
      }
      // option 6: FT0C + FV0
      ampl6[0] = sumAmpFT0C;
      ampl6[1] = sumAmpFV0;
      if (sumAmpFT0C > 0 && sumAmpFV0 > 0) {
        isOK_estimator6 = true;
      }
      if (isOK_estimator6) {
        if (applyNorm) {
          all_weights = 0;
          for (int i_6 = 0; i_6 < nEta6; ++i_6) {
            combined_estimator6 +=
              ampl6[i_6] * weigthsEta6[i_6] / deltaEeta6[i_6];
            all_weights += weigthsEta6[i_6];
          }
          combined_estimator6 /= all_weights;
        } else {
          for (int i_6 = 0; i_6 < nEta6; ++i_6) {
            combined_estimator6 += ampl6[i_6] * weigthsEta6[i_6];
          }
        }
      }
      if (flattenicityQA) {
        rFlattenicity.fill(HIST("hFT0Aampl"), sumAmpFT0A);
        rFlattenicity.fill(HIST("hFT0Campl"), sumAmpFT0C);
        rFlattenicity.fill(HIST("hFV0amplRing1to4"), sumAmpFV01to4Ch);
        rFlattenicity.fill(HIST("hEv"), 4);
      }
      estimator[0] = multGlob;
      estimator[1] = sumAmpFV0;
      estimator[2] = 1.0 - flattenicityfv0;
      estimator[3] = combined_estimator5;
      float flatenicity_ft0 = (flatenicity_t0a + flatenicity_t0c) / 2.0;
      estimator[4] = 1.0 - flatenicity_ft0;
      estimator[5] = combined_estimator6;
      float flatenicity_ft0v0 = 0.5 * flattenicityfv0 + 0.5 * flatenicity_t0c;
      estimator[6] = 1.0 - flatenicity_ft0v0;
      estimator[7] = ptT;
      if (flattenicityQA) {
        rFlattenicity.fill(HIST(nhEst[0]), estimator[0], estimator[0]);
        rFlattenicity.fill(HIST(nhEst[1]), estimator[1], estimator[0]);
        rFlattenicity.fill(HIST(nhEst[2]), estimator[2], estimator[0]);
        rFlattenicity.fill(HIST(nhEst[3]), estimator[3], estimator[0]);
        rFlattenicity.fill(HIST(nhEst[4]), estimator[4], estimator[0]);
        rFlattenicity.fill(HIST(nhEst[5]), estimator[5], estimator[0]);
        rFlattenicity.fill(HIST(nhEst[6]), estimator[6], estimator[0]);
        rFlattenicity.fill(HIST(nhEst[7]), estimator[7], estimator[0]);

        // plot pt vs estimators
        for (auto& track : tracks) {
          if (!track.isGlobalTrack()) {
            continue;
          }
          float pt = track.pt();
          rFlattenicity.fill(HIST(nhPtEst[0]), estimator[0], pt);
          rFlattenicity.fill(HIST(nhPtEst[1]), estimator[1], pt);
          rFlattenicity.fill(HIST(nhPtEst[2]), estimator[2], pt);
          rFlattenicity.fill(HIST(nhPtEst[3]), estimator[3], pt);
          rFlattenicity.fill(HIST(nhPtEst[4]), estimator[4], pt);
          rFlattenicity.fill(HIST(nhPtEst[5]), estimator[5], pt);
          rFlattenicity.fill(HIST(nhPtEst[6]), estimator[6], pt);
          rFlattenicity.fill(HIST(nhPtEst[7]), estimator[7], pt);
        }

        if (isflattenicitywithFV0) {
          for (int iCh = 0; iCh < 48; ++iCh) {
            rFlattenicity.fill(HIST("hAmpV0VsCh"), iCh, ampchannel[iCh]);
            rFlattenicity.fill(HIST("hAmpV0VsChBeforeCalibration"), iCh,
                               ampchannelBefore[iCh]);
          }
        }

        rFlattenicity.fill(HIST("fMultFv0"), sumAmpFV0);
        rFlattenicity.fill(HIST("hFlatFT0CvsFlatFT0A"), flatenicity_t0c,
                           flatenicity_t0a);
      }
    }
    float finalflattenicity = estimator[2];
    if (flattenicityforanalysis == 1) {
      finalflattenicity = estimator[4];
    }
    if (flattenicityforanalysis == 2) {
      finalflattenicity = estimator[6];
    }
    return finalflattenicity;
  }

  template <typename McParticles>
  float EstimateFlattenicityFV0MC(McParticles const& mcParticles)
  {
    RhoLatticeFV0AMC.fill(0);
    float flattenicity = -1;
    float etamin, etamax, minphi, maxphi, dphi;
    int isegment = 0, nsectors;
    for (const auto& mcParticle : mcParticles) {
      if (!(mcParticle.isPhysicalPrimary() && mcParticle.pt() > 0)) {
        continue;
      }

      auto pdgParticle = pdg->GetParticle(mcParticle.pdgCode());
      if (!(pdgParticle && pdgParticle->Charge() > 0.01)) {
        continue;
      }

      float etap = mcParticle.eta();
      float phip = mcParticle.phi();
      isegment = 0;
      for (int ieta = 0; ieta < 5; ieta++) {
        etamax = maxEtaFV0 - ieta * detaFV0;
        if (ieta == 0) {
          etamax = maxEtaFV0;
        }
        etamin = maxEtaFV0 - (ieta + 1) * detaFV0;
        if (ieta == 4) {
          etamin = minEtaFV0;
        }
        nsectors = 8;
        if (ieta == 4) {
          nsectors = 16;
        }
        for (int iphi = 0; iphi < nsectors; iphi++) {
          minphi = iphi * 2.0 * TMath::Pi() / nsectors;
          maxphi = (iphi + 1) * 2.0 * TMath::Pi() / nsectors;
          dphi = TMath::Abs(maxphi - minphi);
          if (etap >= etamin && etap < etamax && phip >= minphi && phip < maxphi) {
            RhoLatticeFV0AMC[isegment] += 1.0 / TMath::Abs(dphi * detaFV0);
          }
          isegment++;
        }
      }
    }

    flattenicity =
      1.0 - GetFlatenicity({RhoLatticeFV0AMC.data(), RhoLatticeFV0AMC.size()});
    return flattenicity;
  }
  // ====================== Flattenicity estimation ends =====================

  // Filters on V0s
  // Cannot filter on dynamic columns, so we cut on DCA to PV and DCA between
  // daughters only
  Filter preFilterV0 = (nabs(aod::v0data::dcapostopv) > v0setting_dcapostopv &&
                        nabs(aod::v0data::dcanegtopv) > v0setting_dcanegtopv &&
                        aod::v0data::dcaV0daughters < v0setting_dcav0dau);

  Filter trackFilter =
    (nabs(aod::track::eta) < cfgTrkEtaCut && aod::track::pt > cfgTrkLowPtCut);

  using TrackCandidates = soa::Filtered<
    soa::Join<aod::TracksIU, aod::TracksExtra, aod::TracksDCA,
              aod::TrackSelection, aod::pidTPCPi, aod::pidTPCPr>>;

  void processDataRun3(
    soa::Join<aod::Collisions, aod::EvSels,
              aod::PVMults>::iterator const& collision,
    soa::Filtered<aod::V0Datas> const& V0s, TrackCandidates const& tracks,
    soa::Join<aod::BCs, aod::Timestamps> const& /*bcs*/,
    aod::MFTTracks const& /*mfttracks*/, aod::FT0s const& /*ft0s*/,
    aod::FV0As const& /*fv0s*/)
  {
    if (applyEvSel &&
        !(isEventSelected(collision))) { // Checking if the event passes the
                                         // selection criteria
      return;
    }

    auto vtxZ = collision.posZ();
    auto vtxY = collision.posY();
    auto vtxX = collision.posX();

    float flattenicity = EstimateFlattenicity(collision, tracks);

    rEventSelection.fill(HIST("hVertexZ"), vtxZ);
    rEventSelection.fill(HIST("hFlattenicityDistribution"), flattenicity);

    for (const auto& v0 : V0s) {
      const auto& posDaughterTrack = v0.posTrack_as<TrackCandidates>();
      const auto& negDaughterTrack = v0.negTrack_as<TrackCandidates>();

      if (TMath::Abs(posDaughterTrack.eta()) > cfgTrkEtaCut ||
          TMath::Abs(negDaughterTrack.eta()) > cfgTrkEtaCut ||
          negDaughterTrack.pt() < cfgTrkLowPtCut ||
          posDaughterTrack.pt() < cfgTrkLowPtCut) {
        continue;
      }
      float massK0s = v0.mK0Short();
      float massLambda = v0.mLambda();
      float massAntiLambda = v0.mAntiLambda();

      rKzeroShort.fill(HIST("hMassK0s"), massK0s);
      rLambda.fill(HIST("hMassLambda"), massLambda);
      rAntiLambda.fill(HIST("hMassAntiLambda"), massAntiLambda);

      float decayvtxX = v0.x();
      float decayvtxY = v0.y();
      float decayvtxZ = v0.z();

      float decaylength = TMath::Sqrt(TMath::Power(decayvtxX - vtxX, 2) +
                                      TMath::Power(decayvtxY - vtxY, 2) +
                                      TMath::Power(decayvtxZ - vtxZ, 2));
      float v0p = TMath::Sqrt(v0.pt() * v0.pt() + v0.pz() * v0.pz());

      float ctauK0s = decaylength * massK0s / v0p;
      float ctauLambda = decaylength * massLambda / v0p;
      float ctauAntiLambda = decaylength * massAntiLambda / v0p;

      // Cut on dynamic columns for K0s

      if (v0.v0cosPA() >= v0setting_cospaK0s &&
          v0.v0radius() >= v0setting_radiusK0s &&
          TMath::Abs(posDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
          TMath::Abs(negDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
          ctauK0s < v0setting_ctauK0s &&
          TMath::Abs(v0.rapidity(0)) <= v0setting_rapidity &&
          TMath::Abs(massLambda - pdgmassLambda) > v0setting_massrejectionK0s &&
          TMath::Abs(massAntiLambda - pdgmassLambda) >
            v0setting_massrejectionK0s) {

        rKzeroShort.fill(HIST("hMassK0sSelected"), massK0s);
        rKzeroShort.fill(HIST("hDCAV0DaughtersK0s"), v0.dcaV0daughters());
        rKzeroShort.fill(HIST("hV0CosPAK0s"), v0.v0cosPA());
        rKzeroShort.fill(HIST("hrapidityK0s"), v0.rapidity(0));
        rKzeroShort.fill(HIST("hctauK0s"), ctauK0s);
        rKzeroShort.fill(HIST("h2DdecayRadiusK0s"), v0.v0radius());
        rKzeroShort.fill(HIST("hMassK0spT"), massK0s, v0.pt());
        rKzeroShort.fill(HIST("hMassK0spTFlat"), massK0s, v0.pt(), flattenicity);

        // Filling the PID of the V0 daughters in the region of the K0s peak
        if (0.45 < massK0s && massK0s < 0.55) {
          rKzeroShort.fill(HIST("hNSigmaPosPionFromK0s"),
                           posDaughterTrack.tpcNSigmaPi(),
                           posDaughterTrack.tpcInnerParam());
          rKzeroShort.fill(HIST("hNSigmaNegPionFromK0s"),
                           negDaughterTrack.tpcNSigmaPi(),
                           negDaughterTrack.tpcInnerParam());
        }
      }

      // Cut on dynamic columns for Lambda
      if (v0.v0cosPA() >= v0setting_cospaLambda &&
          v0.v0radius() >= v0setting_radiusLambda &&
          TMath::Abs(posDaughterTrack.tpcNSigmaPr()) <= NSigmaTPCProton &&
          TMath::Abs(negDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
          ctauLambda < v0setting_ctauLambda &&
          TMath::Abs(v0.rapidity(1)) <= v0setting_rapidity &&
          TMath::Abs(massK0s - pdgmassK0s) > v0setting_massrejectionLambda) {

        rLambda.fill(HIST("hMassLambdaSelected"), massLambda);
        rLambda.fill(HIST("hDCAV0DaughtersLambda"), v0.dcaV0daughters());
        rLambda.fill(HIST("hV0CosPALambda"), v0.v0cosPA());
        rLambda.fill(HIST("hrapidityLambda"), v0.rapidity(1));
        rLambda.fill(HIST("hctauLambda"), ctauLambda);
        rLambda.fill(HIST("h2DdecayRadiusLambda"), v0.v0radius());
        rLambda.fill(HIST("hMassLambdapT"), massLambda, v0.pt());
        rLambda.fill(HIST("hMassLambdapTFlat"), massLambda, v0.pt(), flattenicity);

        // Filling the PID of the V0 daughters in the region of the Lambda peak
        if (1.015 < massLambda && massLambda < 1.215) {
          rLambda.fill(HIST("hNSigmaPosPionFromLambda"),
                       posDaughterTrack.tpcNSigmaPr(),
                       posDaughterTrack.tpcInnerParam());
          rLambda.fill(HIST("hNSigmaNegPionFromLambda"),
                       negDaughterTrack.tpcNSigmaPi(),
                       negDaughterTrack.tpcInnerParam());
        }
      }

      // Cut on dynamic columns for AntiLambda
      if (v0.v0cosPA() >= v0setting_cospaLambda &&
          v0.v0radius() >= v0setting_radiusLambda &&
          TMath::Abs(posDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
          TMath::Abs(negDaughterTrack.tpcNSigmaPr()) <= NSigmaTPCProton &&
          ctauAntiLambda < v0setting_ctauLambda &&
          TMath::Abs(v0.rapidity(2)) <= v0setting_rapidity &&
          TMath::Abs(massK0s - pdgmassK0s) > v0setting_massrejectionLambda) {

        rAntiLambda.fill(HIST("hMassAntiLambdaSelected"), massAntiLambda);
        rAntiLambda.fill(HIST("hDCAV0DaughtersAntiLambda"),
                         v0.dcaV0daughters());
        rAntiLambda.fill(HIST("hV0CosPAAntiLambda"), v0.v0cosPA());
        rAntiLambda.fill(HIST("hrapidityAntiLambda"), v0.rapidity(2));
        rAntiLambda.fill(HIST("hctauAntiLambda"), ctauAntiLambda);
        rAntiLambda.fill(HIST("h2DdecayRadiusAntiLambda"), v0.v0radius());
        rAntiLambda.fill(HIST("hMassAntiLambdapT"), massAntiLambda, v0.pt());

        rAntiLambda.fill(HIST("hMassAntiLambdapTFlat"), massAntiLambda, v0.pt(), flattenicity);
        // Filling the PID of the V0 daughters in the region of the AntiLambda
        // peak
        if (1.015 < massAntiLambda && massAntiLambda < 1.215) {
          rAntiLambda.fill(HIST("hNSigmaPosPionFromAntiLambda"),
                           posDaughterTrack.tpcNSigmaPi(),
                           posDaughterTrack.tpcInnerParam());
          rAntiLambda.fill(HIST("hNSigmaNegPionFromAntiLambda"),
                           negDaughterTrack.tpcNSigmaPr(),
                           negDaughterTrack.tpcInnerParam());
        }
      }
    }
  }

  using TrackCandidatesMC =
    soa::Filtered<soa::Join<aod::TracksIU, aod::TracksExtra, aod::TracksDCA,
                            aod::TrackSelection, aod::pidTPCPi, aod::pidTPCPr,
                            aod::McTrackLabels>>;

  Preslice<soa::Filtered<soa::Join<aod::V0Datas, aod::McV0Labels>>> perCol = aod::track::collisionId;
  Preslice<aod::McParticles> perMCCol = aod::mcparticle::mcCollisionId;
  SliceCache cache1;

  void processRecMC(
    soa::Join<aod::Collisions, aod::EvSels,
              aod::PVMults, aod::McCollisionLabels> const& collisions,
    soa::Filtered<soa::Join<aod::V0Datas, aod::McV0Labels>> const& V0s, aod::McCollisions const&, TrackCandidatesMC const& tracks,
    soa::Join<aod::BCs, aod::Timestamps> const& /*bcs*/,
    aod::MFTTracks const& /*mfttracks*/, aod::FT0s const& /*ft0s*/,
    aod::FV0As const& /*fv0s*/, aod::McParticles const& mcParticles)
  {
    for (const auto& collision : collisions) {
      if (applyEvSel &&
          !(isEventSelected(collision))) { // Checking if the event passes the
                                           // selection criteria
        continue;
      }

      auto vtxZ = collision.posZ();
      auto vtxY = collision.posY();
      auto vtxX = collision.posX();

      float flattenicity = EstimateFlattenicity(collision, tracks);

      rEventSelection.fill(HIST("hVertexZ"), vtxZ);
      rEventSelection.fill(HIST("hFlattenicityDistribution"), flattenicity);

      auto v0sThisCollision = V0s.sliceBy(perCol, collision.globalIndex());
      const auto& mcCollision = collision.mcCollision_as<aod::McCollisions>();

      for (const auto& v0 : v0sThisCollision) {

        const auto& posDaughterTrack = v0.posTrack_as<TrackCandidatesMC>();
        const auto& negDaughterTrack = v0.negTrack_as<TrackCandidatesMC>();

        if (TMath::Abs(posDaughterTrack.eta()) > cfgTrkEtaCut ||
            TMath::Abs(negDaughterTrack.eta()) > cfgTrkEtaCut ||
            negDaughterTrack.pt() < cfgTrkLowPtCut ||
            posDaughterTrack.pt() < cfgTrkLowPtCut) {
          continue;
        }

        if (!v0.has_mcParticle()) {
          continue;
        }

        float massK0s = v0.mK0Short();
        float massLambda = v0.mLambda();
        float massAntiLambda = v0.mAntiLambda();

        rKzeroShort.fill(HIST("hMassK0s"), massK0s);
        rLambda.fill(HIST("hMassLambda"), massLambda);
        rAntiLambda.fill(HIST("hMassAntiLambda"), massAntiLambda);

        float decayvtxX = v0.x();
        float decayvtxY = v0.y();
        float decayvtxZ = v0.z();

        float decaylength = TMath::Sqrt(TMath::Power(decayvtxX - vtxX, 2) +
                                        TMath::Power(decayvtxY - vtxY, 2) +
                                        TMath::Power(decayvtxZ - vtxZ, 2));
        float v0p = TMath::Sqrt(v0.pt() * v0.pt() + v0.pz() * v0.pz());

        float ctauK0s = decaylength * massK0s / v0p;
        float ctauLambda = decaylength * massLambda / v0p;
        float ctauAntiLambda = decaylength * massAntiLambda / v0p;
        auto v0mcParticle = v0.mcParticle();
        // Cut on dynamic columns for K0s

        if (v0mcParticle.pdgCode() == 310 && v0.v0cosPA() >= v0setting_cospaK0s &&
            v0.v0radius() >= v0setting_radiusK0s &&
            TMath::Abs(posDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
            TMath::Abs(negDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
            ctauK0s < v0setting_ctauK0s &&
            TMath::Abs(v0.rapidity(0)) <= v0setting_rapidity &&
            TMath::Abs(massLambda - pdgmassLambda) > v0setting_massrejectionK0s &&
            TMath::Abs(massAntiLambda - pdgmassLambda) >
              v0setting_massrejectionK0s) {

          rKzeroShort.fill(HIST("hMassK0sSelected"), massK0s);
          rKzeroShort.fill(HIST("hDCAV0DaughtersK0s"), v0.dcaV0daughters());
          rKzeroShort.fill(HIST("hV0CosPAK0s"), v0.v0cosPA());
          rKzeroShort.fill(HIST("hrapidityK0s"), v0.rapidity(0));
          rKzeroShort.fill(HIST("hctauK0s"), ctauK0s);
          rKzeroShort.fill(HIST("h2DdecayRadiusK0s"), v0.v0radius());
          rKzeroShort.fill(HIST("hMassK0spT"), massK0s, v0.pt());
          rKzeroShort.fill(HIST("hMassK0spTFlat"), massK0s, v0.pt(), flattenicity);

          // Filling the PID of the V0 daughters in the region of the K0s peak
          if (0.45 < massK0s && massK0s < 0.55) {
            rKzeroShort.fill(HIST("hNSigmaPosPionFromK0s"),
                             posDaughterTrack.tpcNSigmaPi(),
                             posDaughterTrack.tpcInnerParam());
            rKzeroShort.fill(HIST("hNSigmaNegPionFromK0s"),
                             negDaughterTrack.tpcNSigmaPi(),
                             negDaughterTrack.tpcInnerParam());
          }
        }

        // Cut on dynamic columns for Lambda
        if (v0mcParticle.pdgCode() == 3122 &&
            v0.v0cosPA() >= v0setting_cospaLambda &&
            v0.v0radius() >= v0setting_radiusLambda &&
            TMath::Abs(posDaughterTrack.tpcNSigmaPr()) <= NSigmaTPCProton &&
            TMath::Abs(negDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
            ctauLambda < v0setting_ctauLambda &&
            TMath::Abs(v0.rapidity(1)) <= v0setting_rapidity &&
            TMath::Abs(massK0s - pdgmassK0s) > v0setting_massrejectionLambda) {

          rLambda.fill(HIST("hMassLambdaSelected"), massLambda);
          rLambda.fill(HIST("hDCAV0DaughtersLambda"), v0.dcaV0daughters());
          rLambda.fill(HIST("hV0CosPALambda"), v0.v0cosPA());
          rLambda.fill(HIST("hrapidityLambda"), v0.rapidity(1));
          rLambda.fill(HIST("hctauLambda"), ctauLambda);
          rLambda.fill(HIST("h2DdecayRadiusLambda"), v0.v0radius());
          rLambda.fill(HIST("hMassLambdapT"), massLambda, v0.pt());
          rLambda.fill(HIST("hMassLambdapTFlat"), massLambda, v0.pt(), flattenicity);

          // Filling the PID of the V0 daughters in the region of the Lambda peak
          if (1.015 < massLambda && massLambda < 1.215) {
            rLambda.fill(HIST("hNSigmaPosPionFromLambda"),
                         posDaughterTrack.tpcNSigmaPr(),
                         posDaughterTrack.tpcInnerParam());
            rLambda.fill(HIST("hNSigmaNegPionFromLambda"),
                         negDaughterTrack.tpcNSigmaPi(),
                         negDaughterTrack.tpcInnerParam());
          }
        }

        // Cut on dynamic columns for AntiLambda
        if (v0mcParticle.pdgCode() == -3122 &&
            v0.v0cosPA() >= v0setting_cospaLambda &&
            v0.v0radius() >= v0setting_radiusLambda &&
            TMath::Abs(posDaughterTrack.tpcNSigmaPi()) <= NSigmaTPCPion &&
            TMath::Abs(negDaughterTrack.tpcNSigmaPr()) <= NSigmaTPCProton &&
            ctauAntiLambda < v0setting_ctauLambda &&
            TMath::Abs(v0.rapidity(2)) <= v0setting_rapidity &&
            TMath::Abs(massK0s - pdgmassK0s) > v0setting_massrejectionLambda) {

          rAntiLambda.fill(HIST("hMassAntiLambdaSelected"), massAntiLambda);
          rAntiLambda.fill(HIST("hDCAV0DaughtersAntiLambda"),
                           v0.dcaV0daughters());
          rAntiLambda.fill(HIST("hV0CosPAAntiLambda"), v0.v0cosPA());
          rAntiLambda.fill(HIST("hrapidityAntiLambda"), v0.rapidity(2));
          rAntiLambda.fill(HIST("hctauAntiLambda"), ctauAntiLambda);
          rAntiLambda.fill(HIST("h2DdecayRadiusAntiLambda"), v0.v0radius());
          rAntiLambda.fill(HIST("hMassAntiLambdapT"), massAntiLambda, v0.pt());
          rAntiLambda.fill(HIST("hMassAntiLambdapTFlat"), massAntiLambda, v0.pt(), flattenicity);

          // Filling the PID of the V0 daughters in the region of the AntiLambda
          // peak
          if (1.015 < massAntiLambda && massAntiLambda < 1.215) {
            rAntiLambda.fill(HIST("hNSigmaPosPionFromAntiLambda"),
                             posDaughterTrack.tpcNSigmaPi(),
                             posDaughterTrack.tpcInnerParam());
            rAntiLambda.fill(HIST("hNSigmaNegPionFromAntiLambda"),
                             negDaughterTrack.tpcNSigmaPr(),
                             negDaughterTrack.tpcInnerParam());
          }
        }
      }

      const auto particlesInCollision = mcParticles.sliceByCached(aod::mcparticle::mcCollisionId, mcCollision.globalIndex(), cache1);
      float flattenicityMCGen = EstimateFlattenicityFV0MC(particlesInCollision);
      rEventSelection.fill(HIST("hFlattenicityDistributionMCGen_Rec"), flattenicityMCGen);
      rEventSelection.fill(HIST("hFlattenicity_Corr_Gen_vs_Rec"), flattenicityMCGen, flattenicity);

      for (auto& mcParticle : particlesInCollision) {
        if (!mcParticle.isPhysicalPrimary()) {
          continue;
        }

        if (std::abs(mcParticle.y()) > 0.5f) {
          continue;
        }

        if (mcParticle.pdgCode() == 310) {
          rKzeroShort.fill(HIST("Generated_MCRecoCollCheck_INEL_K0Short"), mcParticle.pt(), flattenicity); // K0s
        }
        if (mcParticle.pdgCode() == 3122) {
          rLambda.fill(HIST("Generated_MCRecoCollCheck_INEL_Lambda"), mcParticle.pt(), flattenicity); // Lambda
        }
        if (mcParticle.pdgCode() == -3122) {
          rAntiLambda.fill(HIST("Generated_MCRecoCollCheck_INEL_AntiLambda"), mcParticle.pt(), flattenicity); // AntiLambda
        }
      }
    }
  }

  // Filter posZFilterMC = (nabs(o2::aod::mccollision::posZ) < cutzvertex);
  void processGenMC(
    o2::aod::McCollision const& mcCollision,
    const soa::SmallGroups<soa::Join<
      o2::aod::Collisions, o2::aod::McCollisionLabels, o2::aod::EvSels, aod::PVMults>>& collisions,
    o2::aod::McParticles const& mcParticles)
  {
    float flattenicity = EstimateFlattenicityFV0MC(mcParticles);
    rEventSelection.fill(HIST("hFlattenicityDistributionMCGen"), flattenicity);
    //====================================
    //===== Event Loss Denominator =======
    //====================================

    rEventSelection.fill(HIST("hNEventsMCGen"), 0.5);

    if (TMath::Abs(mcCollision.posZ()) > cutzvertex) {
      return;
    }
    rEventSelection.fill(HIST("hNEventsMCGen"), 1.5);
    rEventSelection.fill(HIST("hFlat_GenColl_MC"), flattenicity);

    bool isINELgt0true = false;

    if (pwglf::isINELgtNmc(mcParticles, 0, pdg)) {
      isINELgt0true = true;
      rEventSelection.fill(HIST("hNEventsMCGen"), 2.5);
      rEventSelection.fill(HIST("hFlat_GenColl_MC_INELgt0"), flattenicity);
    }

    //=====================================
    //===== Signal Loss Denominator =======
    //=====================================

    for (auto& mcParticle : mcParticles) {

      if (!mcParticle.isPhysicalPrimary()) {
        continue;
      }
      if (std::abs(mcParticle.y()) > 0.5f) {
        continue;
      }

      if (mcParticle.pdgCode() == 310) {
        rKzeroShort.fill(HIST("pGen_MCGenColl_INEL_K0Short"), mcParticle.pt(), flattenicity); // K0s
        if (isINELgt0true) {
          rKzeroShort.fill(HIST("pGen_MCGenColl_INELgt0_K0Short"), mcParticle.pt(), flattenicity); // K0s
        }
      }
      if (mcParticle.pdgCode() == 3122) {
        rLambda.fill(HIST("pGen_MCGenColl_INEL_Lambda"), mcParticle.pt(), flattenicity); // Lambda
        if (isINELgt0true) {
          rLambda.fill(HIST("pGen_MCGenColl_INELgt0_Lambda"), mcParticle.pt(), flattenicity); // Lambda
        }
      }
      if (mcParticle.pdgCode() == -3122) {
        rAntiLambda.fill(HIST("pGen_MCGenColl_INEL_AntiLambda"), mcParticle.pt(), flattenicity); // AntiLambda
        if (isINELgt0true) {
          rAntiLambda.fill(HIST("pGen_MCGenColl_INELgt0_AntiLambda"), mcParticle.pt(), flattenicity); // AntiLambda
        }
      }
    }

    int recoCollIndex_INEL = 0;
    int recoCollIndex_INELgt0 = 0;
    for (auto& collision : collisions) { // loop on reconstructed collisions

      //=====================================
      //====== Event Split Numerator ========
      //=====================================

      rEventSelection.fill(HIST("hNEventsMCReco"), 0.5);
      if (applyEvSel && !isEventSelected(collision)) {
        continue;
      }
      rEventSelection.fill(HIST("hEventsSelected"), 10.5);
      rEventSelection.fill(HIST("hNEventsMCReco"), 1.5);
      rEventSelection.fill(HIST("hFlat_RecoColl_MC"), flattenicity);

      recoCollIndex_INEL++;

      if (collision.isInelGt0() && isINELgt0true) {
        rEventSelection.fill(HIST("hNEventsMCReco"), 2.5);
        rEventSelection.fill(HIST("hFlat_RecoColl_MC_INELgt0"), flattenicity);

        recoCollIndex_INELgt0++;
      }

      //=====================================
      //======== Sgn Split Numerator ========
      //=====================================

      for (auto& mcParticle : mcParticles) {

        if (!mcParticle.isPhysicalPrimary()) {
          continue;
        }

        if (std::abs(mcParticle.y()) > 0.5f) {
          continue;
        }

        if (mcParticle.pdgCode() == 310) {
          rKzeroShort.fill(HIST("Generated_MCRecoColl_INEL_K0Short"), mcParticle.pt(), flattenicity); // K0s
          if (recoCollIndex_INELgt0 > 0) {
            rKzeroShort.fill(HIST("Generated_MCRecoColl_INELgt0_K0Short"), mcParticle.pt(), flattenicity); // K0s
          }
        }
        if (mcParticle.pdgCode() == 3122) {
          rLambda.fill(HIST("Generated_MCRecoColl_INEL_Lambda"), mcParticle.pt(), flattenicity); // Lambda
          if (recoCollIndex_INELgt0 > 0) {
            rLambda.fill(HIST("Generated_MCRecoColl_INELgt0_Lambda"), mcParticle.pt(), flattenicity); // Lambda
          }
        }
        if (mcParticle.pdgCode() == -3122) {
          rAntiLambda.fill(HIST("Generated_MCRecoColl_INEL_AntiLambda"), mcParticle.pt(), flattenicity); // AntiLambda
          if (recoCollIndex_INELgt0 > 0) {
            rAntiLambda.fill(HIST("Generated_MCRecoColl_INELgt0_AntiLambda"), mcParticle.pt(), flattenicity); // AntiLambda
          }
        }
      }
    }

    // From now on keep only mc collisions with at least one reconstructed collision (INEL)
    if (recoCollIndex_INEL < 1) {
      return;
    }

    //=====================================
    //====== Event Loss Numerator =========
    //=====================================

    rEventSelection.fill(HIST("hNEventsMCGenReco"), 0.5);
    rEventSelection.fill(HIST("hFlat_GenRecoColl_MC"), flattenicity);

    if (recoCollIndex_INELgt0 > 0) {
      rEventSelection.fill(HIST("hNEventsMCGenReco"), 1.5);
      rEventSelection.fill(HIST("hFlat_GenRecoColl_MC_INELgt0"), flattenicity);
    }

    //=====================================
    //===== Signal Loss Numerator =========
    //=====================================

    for (auto& mcParticle : mcParticles) {

      if (!mcParticle.isPhysicalPrimary()) {
        continue;
      }

      if (std::abs(mcParticle.y()) > 0.5f) {
        continue;
      }

      if (mcParticle.pdgCode() == 310) {
        rKzeroShort.fill(HIST("pGen_MCGenRecoColl_INEL_K0Short"), mcParticle.pt(), flattenicity); // K0s
        if (recoCollIndex_INELgt0 > 0) {
          rKzeroShort.fill(HIST("pGen_MCGenRecoColl_INELgt0_K0Short"), mcParticle.pt(), flattenicity); // K0s
        }
      }
      if (mcParticle.pdgCode() == 3122) {
        rLambda.fill(HIST("pGen_MCGenRecoColl_INEL_Lambda"), mcParticle.pt(), flattenicity); // Lambda
        if (recoCollIndex_INELgt0 > 0) {
          rLambda.fill(HIST("pGen_MCGenRecoColl_INELgt0_Lambda"), mcParticle.pt(), flattenicity); // Lambda
        }
      }
      if (mcParticle.pdgCode() == -3122) {
        rAntiLambda.fill(HIST("pGen_MCGenRecoColl_INEL_AntiLambda"), mcParticle.pt(), flattenicity); // AntiLambda
        if (recoCollIndex_INELgt0 > 0) {
          rAntiLambda.fill(HIST("pGen_MCGenRecoColl_INELgt0_AntiLambda"), mcParticle.pt(), flattenicity); // AntiLambda
        }
      }
    }
  }

  PROCESS_SWITCH(lambdak0sflattenicity, processDataRun3, "Process Run 3 Data",
                 false);
  PROCESS_SWITCH(lambdak0sflattenicity, processRecMC,
                 "Process Run 3 mc, reconstructed", true);
  PROCESS_SWITCH(lambdak0sflattenicity, processGenMC,
                 "Process Run 3 mc, generated", true);
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{adaptAnalysisTask<lambdak0sflattenicity>(cfgc)};
}
