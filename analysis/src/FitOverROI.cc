#include "FitOverROI.hh"

#include "TMath.h"
#include "TROOT.h"
#include "TFitResult.h"
#include "TSpectrum.h"
#include "TTree.h"
#include "TFile.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLine.h"
#include "TAxis.h"

#include "TList.h"
#include "TCut.h"
#include <algorithm>
#include <numeric>
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "TMatrixDSym.h"
#include "TGraph.h"
#include <fstream>


using namespace std;

#define HISTOGRAMWIDTH 7
#define NPEAKS 7



#define mCON params[CONSTANT]
#define mLAM params[LAMBDA]
#define mMEAN params[MEAN]
#define mSIG params[SIGMA]
#define mAMP params[AMP_E]
#define mPE params[P_E]
#define mSHT params[SHOTNOISE]
#define mPDM params[PEDMEAN]


const char* names[] = {"CONSTANT","LAMBDA", "SPE_MEAN", "SPE_SIGMA", "AMP_E", "P_E", "SHOTNOISE" ,"PEDMEAN","NPAR"};


Double_t FitOverROI::response_0(Double_t* x, Double_t* params) 
{
    double y = x[0] - mPDM;
    return mCON*TMath::Poisson(0,mLAM)*TMath::Gaus(y, 0, mSHT, true);
}

Double_t FitOverROI::background_func(Double_t* x, Double_t* params)
{
    double y = x[0] - mPDM;

    //mathematica version (my own convolution)
    double exp_term=(TMath::Power(TMath::E(),(-2*mAMP*y + mSHT*mSHT)/
				  (2.*mAMP*mAMP))*mPE*
		     (1 + TMath::Erf(((mAMP*y)/mSHT - mSHT)/(sqrt(2)*mAMP))))/
	(2.*mAMP);
    return mCON*TMath::Poisson(1,mLAM)*exp_term;
}

Double_t FitOverROI::gauss_func(Double_t* x, Double_t* params)
{
    double y = x[0] - mPDM;
    double sigma_1=sqrt(mSHT*mSHT + mSIG*mSIG);	
	
    double gauss_term= (1-mPE)*(TMath::Power(TMath::E(), - (mMEAN-y)*(mMEAN-y)/(2*sigma_1*sigma_1)) *
				(1 + TMath::Erf((mMEAN*mSHT*mSHT + mSIG*mSIG*y)/(sqrt(2) * mSIG*mSHT *sigma_1))) /
				(sqrt(2*TMath::Pi())*sigma_1 * (1+ TMath::Erf(mMEAN/(sqrt(2)*mSIG)))));
	
	
    double response=mCON*TMath::Poisson(1,mLAM)*gauss_term;
    return response;
}  
  
Double_t FitOverROI::response_1(Double_t* x, Double_t* params)
{
    double exp_term= background_func(x, params);
	
    double gauss_term= gauss_func(x, params);
	
	
    double response=gauss_term+exp_term;
    return response;
}


Double_t FitOverROI::response_2(Double_t* x, Double_t* params)
{
    double y = x[0] - mPDM;
    double response=((TMath::Power(TMath::E(),(-2*mAMP*y + TMath::Power(mSHT,2))/
				   (2.*TMath::Power(mAMP,2)))*mPE*mPE*
		      (mAMP*y - TMath::Power(mSHT,2)))/TMath::Power(mAMP,3) + 
		     (2*TMath::Power(-1 + mPE,2)*sqrt(2/TMath::Pi()))/
		     (TMath::Power(TMath::E(),TMath::Power(-2*mMEAN + y,2)/
				   (2.*(TMath::Power(mSHT,2) + 
					2*TMath::Power(mSIG,2))))*mSHT*
		      sqrt(2/TMath::Power(mSHT,2) + TMath::Power(mSIG,-2))*
		      mSIG*TMath::Power(1 + TMath::Erf(mMEAN/(sqrt(2)*mSIG)),2)) 
		     + (2*TMath::Power(TMath::E(),
				       (2*mAMP*mMEAN - 2*mAMP*y + TMath::Power(mSHT,2) + 
					TMath::Power(mSIG,2))/(2.*TMath::Power(mAMP,2)))*
			(-1 + mPE)*mPE*
			(-1 + TMath::Erf((mAMP*(mMEAN - y) + 
					  TMath::Power(mSHT,2) + TMath::Power(mSIG,2))/
					 (mAMP*sqrt(2*TMath::Power(mSHT,2) + 
						    2*TMath::Power(mSIG,2))))))
		     /(mAMP*(1 + TMath::Erf(mMEAN/(sqrt(2)*mSIG)))));
    
    return TMath::Poisson(2,mLAM)*mCON*response;
    
      /*double sigma_1=sqrt(mSHT*mSHT + mSIG*mSIG);	
      
      double response= TMath::Poisson(2,mLAM)*(mPE*mPE*y/(mAMP*mAMP) * TMath::Power(TMath::E(), -y/mAMP)+ 
      2 * (1-mPE)*mPE/(sqrt(2*TMath::Pi())*sigma_1) * TMath::Power(TMath::E(), -0.5*(y-mMEAN-mAMP)*(y-mMEAN-mAMP)/(sigma_1*sigma_1)) + 
      (1-mPE)*(1-mPE)/(2 *sqrt(TMath::Pi())*sigma_1) *TMath::Power(TMath::E(),-0.5 *(y- 2*mMEAN)*(y- 2*mMEAN)/(sigma_1 * sigma_1 *2)));
      return mCON*response;*/
}
	
Double_t FitOverROI::m_n(const Double_t* params)
{
    return mMEAN + mAMP*mPE - mMEAN*mPE + 
	(sqrt(2/TMath::Pi())*mSIG*
	 (1/(1 + TMath::Erf(mMEAN/(sqrt(2)*mSIG))) + 
	  mPE/(-2 + TMath::Erfc(mMEAN/(sqrt(2)*mSIG)))))/
	TMath::Power(TMath::E(),TMath::Power(mMEAN,2)/(2.*TMath::Power(mSIG,2)));
}

Double_t FitOverROI::sigma_n(const  Double_t* params)
{
    return sqrt(-(TMath::Power(mMEAN,2)*(-1 + mPE)) + 
		2*TMath::Power(mAMP,2)*mPE  - 
		(-1 + mPE)*TMath::Power(mSIG,2) - 
		TMath::Power(mMEAN + mAMP*mPE - mMEAN*mPE + 
			     (sqrt(2/TMath::Pi())*mSIG*
			      (1/(1 + TMath::Erf(mMEAN/(sqrt(2)*mSIG))) + 
			       mPE/(-2 + TMath::Erfc(mMEAN/(sqrt(2)*mSIG)))
				  ))/
			     TMath::Power(TMath::E(),TMath::Power(mMEAN,2)/
					  (2.*TMath::Power(mSIG,2))),2) + 
		(mMEAN*sqrt(2/TMath::Pi())*mSIG*
		 (1/(1 + TMath::Erf(mMEAN/(sqrt(2)*mSIG))) + 
		  mPE/(-2 + TMath::Erfc(mMEAN/(sqrt(2)*mSIG)))))
		/TMath::Power(TMath::E(),TMath::Power(mMEAN,2)/
			      (2.*TMath::Power(mSIG,2))));
}
	   
Double_t FitOverROI::response_multi(Double_t* x, Double_t* params)
{	
    double y = x[0] - mPDM;
	
		
    double response=0;
	
    for(int i=3; i<=NPEAKS; i++)
    {
	response += TMath::Poisson(i,mLAM) *TMath::Gaus(y,m_n(params)*i,sqrt(i*sigma_n(params)*sigma_n(params)+TMath::Power(mSHT,2)),true);
    }
    return mCON*response;
}	

Double_t FitOverROI::SPEFunc(Double_t* x, Double_t* params)
{
    double sig = (response_0(x, params)+response_1(x,params)+response_2(x,params)+response_multi(x,params));	
    /*double fixerparams[] = {mCON, 0.00143997, 83.9085, 29.0772, 149.998, 0.704112 , mSHT };
      double base= (response_1(x,fixerparams)+response_2(x,fixerparams)+response_multi(x,fixerparams));
      return sig*(TMath::Poisson(0,0.00143997))+base;*/
    return sig;

}

TFitResultPtr FitOverROI::FitSPE(FitTH1F* spe, ChanFitSettings& CFS, int ntriggers,
				 bool allow_bg,
				 bool force_old)
{
    
    TF1* spefunc = (TF1*)gROOT->GetFunction("spefunc");
    int nEvtsInRange = (int)spe->Integral(0,spe->GetNbinsX()+1,"width");
	
    spe->GetXaxis()->SetRangeUser(CFS.range_min, CFS.range_max);
	
    double fitmin, fitmax;
    fitmin = spe->GetBinLowEdge(spe->GetXaxis()->GetFirst());
    fitmax = 0;
    //find the last non-zero bin
    int bin = spe->GetXaxis()->GetLast();
    //bin = spe->GetNbinsX()+1;
        	
    while(spe->GetBinContent(bin) == 0) {bin--;}
    fitmax = spe->GetBinLowEdge(bin);
    fitmax = fitmax*1; 

    //Pedestal centering
	
    Double_t pedmean = CFS.pedmean_min_bound;
    if(fitmin<0 && !force_old && (max(fitmin,CFS.pedrange_min)< min(CFS.pedrange_max,fitmax)))
    {
	spe->GetXaxis()->SetRangeUser(max(fitmin,CFS.pedrange_min), min(CFS.pedrange_max,fitmax));
	std::cout<<"Fitting pedestal in range ["<<max(fitmin,CFS.pedrange_min)<<", "<<min(fitmax,CFS.pedrange_max)<<"]"<<std::endl;
	TFitResultPtr pedfit = spe->Fit("gaus","QMIS");
	if(pedfit.Get())
	{
	    pedmean = pedfit->Value(1);
	}
	else{pedmean=0;}
    }
    if(force_old)
    {
	pedmean=spefunc->GetParameter(PEDMEAN);
    }
	
    double params[NPAR];
    //find the likely place for the single p.e. peak
    bin = spe->FindBin(0);
    //params[SHOTNOISE] = FindExtremum(bin, spe, FORWARD, MINIMUM)/3.;


    if(!force_old || !spefunc)
    {
	params[SHOTNOISE] = CFS.shotnoise_start_value;
	params[MEAN] = CFS.mean_start_value;
	params[SIGMA] = CFS.sigma_start_value;
	params[LAMBDA] = CFS.lambda_start_value;
	if(CFS.constant_start_value)
	{ 
	    params[CONSTANT] = nEvtsInRange;
	} 
	else
	{  
	    params[CONSTANT] = CFS.constant_start_value;
	}
	params[AMP_E] = CFS.amp_E_start_value;
	params[P_E] = CFS.p_E_start_value;  
		
	spefunc = new TF1("spefunc", SPEFunc, fitmin, fitmax, NPAR);

	spefunc->SetParameters(params);
	for(int i=0; i<NPAR; i++)
	    spefunc->SetParName(i, names[i]);
	//spefunc->SetParLimits(CONSTANT, 0 , params[CONSTANT]);
	//DIVOT
	spefunc->SetParLimits(LAMBDA, CFS.lambda_min_bound, CFS.lambda_max_bound);
	spefunc->SetParLimits(MEAN, CFS.mean_min_bound, CFS.mean_max_bound);//spe_min, spe_max);
	spefunc->SetParLimits(SHOTNOISE, CFS.shotnoise_min_bound, CFS.shotnoise_max_bound);//spe_min, spe_max);		
	spefunc->SetParLimits(SIGMA, CFS.sigma_min_bound, CFS.sigma_max_bound);
	spefunc->SetParLimits(P_E, CFS.p_E_min_bound, CFS.p_E_max_bound);
	spefunc->SetParLimits(AMP_E, CFS.amp_E_min_bound, CFS.amp_E_max_bound);//Focus here
	if(!allow_bg)
	{
	    cout<<"Disallowing exponential background for this fit"<<endl;
	    spefunc->SetParameter(AMP_E,0);
	    spefunc->SetParameter(P_E,0);
	    spefunc->FixParameter(AMP_E,0);
	    spefunc->FixParameter(P_E,0);
	}
		
		
	spefunc->SetLineStyle(1);
	spefunc->SetLineColor(kBlue);
    }
    else
    {
	for(int i=0; i<NPAR; i++)
	{
	    if(i == CONSTANT || i == LAMBDA)
		continue;
	    double width=0;
	    spefunc->SetParLimits(i,
				  std::max(0. ,spefunc->GetParameter(i)-width*spefunc->GetParError(i)),
				  spefunc->GetParameter(i) + width*spefunc->GetParError(i));
	}
    }
    spefunc->FixParameter(CONSTANT, nEvtsInRange);
    spefunc->FixParameter(PEDMEAN, pedmean);
        
    spe->GetXaxis()->SetRangeUser(fitmin, fitmax);
    
    spe->Draw();
    spefunc->Draw("same");

    //spe->Fit(spefunc,"MRES");
    //spe->Fit(spefunc,"MRL");
    std::cout<<std::endl<<"Fitting entire spectrum"<<std::endl;
    TFitResultPtr fitResult = spe->Fit(spefunc,"QMRES");
    spe->fitResult= fitResult;

    //spefunc->DrawCopy("same");
    std::cout<<endl<<"Fit Results: "<<endl
	     <<"Fit Status: "<<spe->fitResult<<endl 
	     <<"Chi2/NDF = "<<spefunc->GetChisquare()<<"/"<<spefunc->GetNDF()<<endl
	     <<"Prob = "<<spefunc->GetProb()<<std::endl<<std::endl;

    for(int i=0; i<NPAR; i++){	params[i] = spefunc->GetParameter(i);}
    TList* funclist = spe->GetListOfFunctions();

    static TF1* background = new TF1("background",background_func,fitmin,fitmax,NPAR);
    background->SetRange(fitmin, fitmax);
    background->SetLineColor(kRed);
    background->SetParameters(spefunc->GetParameters());
    funclist->Add(background->Clone());
  
    static TF1* gauss_curve = new TF1("gause_curve",gauss_func,fitmin,fitmax,NPAR);
    gauss_curve->SetRange(fitmin, fitmax);
    gauss_curve->SetLineColor(kRed);
    gauss_curve->SetParameters(spefunc->GetParameters());
    funclist->Add(gauss_curve->Clone());


    TF1* response_0_f = (TF1*)gROOT->GetFunction("response_0_f");	
    if(!response_0_f){response_0_f = new TF1("response_0_f",response_0,fitmin,fitmax,NPAR);}
    response_0_f->SetRange(fitmin, fitmax);
    response_0_f->SetLineColor(kGreen); 
    response_0_f->SetParameters(spefunc->GetParameters());
    response_0_f->SetRange(fitmin,70);
    if(response_0_f){funclist->Add(response_0_f->Clone());}
	
    TF1* response_1_f = (TF1*)gROOT->GetFunction("response_1_f");	
    if(!response_1_f){response_1_f = new TF1("response_1_f",response_1,fitmin,fitmax,NPAR);}
    response_1_f->SetRange(fitmin, fitmax);
    response_1_f->SetLineColor(kMagenta); 
    response_1_f->SetParameters(spefunc->GetParameters());
    if(response_1_f){funclist->Add(response_1_f->Clone());}
	
    TF1* response_2_f = (TF1*)gROOT->GetFunction("response_2_f");	
    if(!response_2_f){response_2_f = new TF1("response_2_f",response_2,fitmin,fitmax,NPAR);}
    response_2_f->SetRange(fitmin, fitmax);
    response_2_f->SetLineColor(kGreen); 
    response_2_f->SetParameters(spefunc->GetParameters());
    if(response_2_f){funclist->Add(response_2_f->Clone());}
	
    TF1* response_multi_f = (TF1*)gROOT->GetFunction("response_multi_f");	
    if(!response_multi_f){response_multi_f = new TF1("response_multi_f",response_multi,fitmin,fitmax,NPAR);}
    response_multi_f->SetRange(fitmin, fitmax);
    response_multi_f->SetLineColor(kGreen); 
    response_multi_f->SetParameters(spefunc->GetParameters());
    if(response_multi_f){funclist->Add(response_multi_f->Clone());}
	

    //double thresh = spefunc->GetParameter(MEAN) - 2.*spefunc->GetParameter(SIGMA);
    //TLine* twosigma = new TLine(thresh, spe->GetYaxis()->GetBinLowEdge(1), thresh, 1.1*spe->GetMaximum());
    //twosigma->SetLineColor(kGreen);
    //twosigma->Draw();

    cout<<"Valid Events collected: "<<ntriggers<<std::endl;
    cout<<"Events Passing Cuts: " <<spe->GetEntries()<<std::endl;
    cout<<"Noise fraction: "
	<<spefunc->GetParameter(AMP_E)/spefunc->GetParameter(CONSTANT)
	<<std::endl;
    cout<<"Exponential fraction: "<<spefunc->GetParameter(P_E)<<std::endl;
    cout<<"Average photoelectrons per pulse: "<<spefunc->GetParameter(LAMBDA)
	<<std::endl;
    cout<<"Pedestal Mean: "<<spefunc->GetParameter(PEDMEAN)<<" count*samples"<<endl;
    cout<<"Pedestal Width: "<<spefunc->GetParameter(SHOTNOISE)<<" count*samples"<<endl;
    //double mean = spefunc->GetParameter(MEAN);
	

	
    double conversion = 0.00049 /*V/ct*/ * 4E-9 /*ns/samp*/ * 1./25. /*ohm*/ *
	0.1 /*amplifier*/ * 1.E12 /*pC/C*/;
	
    cout<<"Single photoelectron Peak: "<<m_n(params)<<" count*samples"<<std::endl
	<<"Single photoelectron Width: " <<sigma_n (params)<<" count*samples"<<std::endl
	<<"Single photoelectron Charge: "<<m_n(params)*conversion<<" pC"<<std::endl
	<<"Phototube Gain: "<<m_n(params)*conversion / 1.602E-7<<std::endl;
	
    //afan-----------
    double pdfmean_approx = mPE*mAMP+(1-mPE)*mMEAN;
    //double pdfmean_error_uncorr = sqrt(TMath::Power((mAMP-mMEAN)*spefunc->GetParError(P_E),2) 
    //    				   +TMath::Power((1-mPE)*spefunc->GetParError(MEAN),2)
    //				   +TMath::Power(mPE*spefunc->GetParError(AMP_E),2));
    //double pdfmean_error_corr = (pdfmean_error_uncorr 
    //			     + 2*(mAMP-mMEAN)*(1-mPE)*cov[P_E][MEAN] 
    //			     + 2*(mAMP-mMEAN)*mPE*cov[P_E][AMP_E] 
    //			     + 2*(1-mPE)*mPE*cov[MEAN][AMP_E]);
    cout<<"Approximated pdfmean (using corr): "<<pdfmean_approx
	<<" +- "<<pdfmean_error_corr(fitResult)<<endl;
    return fitResult;
}

double FitOverROI::pdfmean_error_corr(TFitResultPtr& fitresult)
{
	TMatrixDSym cov = fitresult->GetCovarianceMatrix();
  const double* params = fitresult->GetParams();
  const double* errors = fitresult->GetErrors();
  //leaving out small correction factor from cut-off gaussian
    double pdfmean_error_uncorr_sq = TMath::Power((mAMP-mMEAN)*errors[P_E],2)
				       +TMath::Power((1-mPE)*errors[MEAN],2)
				       +TMath::Power(mPE*errors[AMP_E],2);
    /* cout<<endl<<"mean err: "<<errors[MEAN]<<" amp err: "
               <<errors[AMP_E]<<" p_E err: "
               <<errors[P_E]<<" uncorr error: "
               <<sqrt(pdfmean_error_uncorr_sq)
               <<" cov matrix (mean x p_E): "<<2*(mAMP-mMEAN)*(1-mPE)*cov[P_E][MEAN]
               <<" cov matrix (amp_E x p_E): "<<2*(mAMP-mMEAN)*mPE*cov[P_E][AMP_E] 
               <<" cov matrix (mean x amp_E): "<<2*(1-mPE)*mPE*cov[MEAN][AMP_E]
               <<endl;
               cov.Print(); */
    return sqrt(pdfmean_error_uncorr_sq + (2*(mAMP-mMEAN)*(1-mPE)*cov[P_E][MEAN] 
				   + 2*(mAMP-mMEAN)*mPE*cov[P_E][AMP_E] 
				   + 2*(1-mPE)*mPE*cov[MEAN][AMP_E]));
  
  
  
  
}

/* double FitOverROI::pdfmean_error_corr(TFitResultPtr& fitresult)
{
    TMatrixDSym cov = fitresult->GetCovarianceMatrix();
    const double* params = fitresult->GetParams();
    const double* errors = fitresult->GetErrors();
    double pdfmean_error_uncorr = sqrt(TMath::Power((mAMP-mMEAN)*errors[P_E],2)
				       +TMath::Power((1-mPE)*errors[MEAN],2)
				       +TMath::Power(mPE*errors[AMP_E],2));
    cout<<endl<<"pdfmean uncorr:"<<pdfmean_error_uncorr<<endl;
    cout<<"correction: "<<(sqrt(2/TMath::Pi())*mSIG*
	 (1/(1 + TMath::Erf(mMEAN/(sqrt(2)*mSIG))) + 
	  mPE/(-2 + TMath::Erfc(mMEAN/(sqrt(2)*mSIG)))))/
	TMath::Power(TMath::E(),TMath::Power(mMEAN,2)/(2.*TMath::Power(mSIG,2)))<<endl;
    cout<<"term 1: "<<2*(mAMP-mMEAN)*(1-mPE)*cov[P_E][MEAN]<<" term 2: "<<2*(mAMP-mMEAN)*mPE*cov[P_E][AMP_E] <<" term 3: "<<2*(1-mPE)*mPE*cov[MEAN][AMP_E]<<endl;
    return pdfmean_error_uncorr + (2*(mAMP-mMEAN)*(1-mPE)*cov[P_E][MEAN] 
                + 2*(mAMP-mMEAN)*mPE*cov[P_E][AMP_E] 
                + 2*(1-mPE)*mPE*cov[MEAN][AMP_E]);

} */
/* int ProcessSPEFile(const char* fname, Long_t roi = -1, int channel = -1, 
   double emax = -1, bool force_old = false)
   {
   if(!gPad) new TCanvas;
   gPad->SetLogy();
   gPad->SetTitle(fname);
   static bool loaded = false;
   if(!loaded){
   gROOT->ProcessLine(".L lib/libDict.so");
   loaded = true;
   }

   TFile* fin = new TFile(fname);
   if(!fin->IsOpen()){
   std::cerr<<"Unable to open file "<<fname<<std::endl;
   return 1;
   }

   TTree* Events = (TTree*)(fin->Get("Events"));
   if(!Events){
   std::cerr<<"Unable to load Events tree from file "<<fname<<std::endl;
   return 2;
   }
   TString data_source;
   if(roi == -1) data_source = "channels[].pulses.integral";
   else data_source = TString("-channels[].regions[") + roi
   + TString("].integral");
   if( emax < 0){
   Events->Draw(data_source+" >> htemp",data_source+" > 0");
   TH1* htemp = (TH1*)(gROOT->FindObject("htemp"));
   emax = htemp->GetMean()*HISTOGRAMWIDTH;
   }
	
   TCut min_en = (data_source+" > 0").Data();
   char chstring[100];
   sprintf(chstring,data_source+" < %.0f",emax);
   TCut max_en = chstring;
   sprintf(chstring,"channels[].channel_id == %d",channel);
   TCut chan_cut = (channel == -1 ? "" : chstring);

   TCut time_cut = (roi == -1 ? get_time_cut(Events, chan_cut) : "" );

   TCut total_cut = min_en && max_en && time_cut && chan_cut;
	
   Events->Draw(data_source+" >> hspec",total_cut,"e");
   TH1* hspec = (TH1*)(gROOT->FindObject("hspec"));


   FitSPE(hspec, Events->GetEntries("channels[].baseline.found_baseline"),
   force_old);


   return 0;
   } */

