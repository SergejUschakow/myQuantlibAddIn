/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2015 Sebastian Schlenkrich

*/



#include <qlo/templatequasigaussian.hpp>

namespace QuantLibAddin {

	RealQuasiGaussianModel::RealQuasiGaussianModel(
						   const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
						         const QuantLib::Handle<QuantLib::YieldTermStructure>& hYTS,
		                         // number of yield curve factors (excluding stoch. vol)
		                         size_t                                             d,       // (d+1)-dimensional Brownian motion for [x(t), z(t)]^T
		                         // unique grid for time-dependent parameters
		                         const std::vector<QuantLib::Time>&                 times,   // time-grid of left-constant model parameter values
		                         // time-dependent parameters, left-piecewise constant on times_-grid
		                         const std::vector< std::vector<QuantLib::Real> >&  lambda,  // volatility
		                         const std::vector< std::vector<QuantLib::Real> >&  alpha,   // shift
		                         const std::vector< std::vector<QuantLib::Real> >&  b,       // f-weighting
		                         const std::vector<QuantLib::Real>&                 eta,     // vol-of-vol
		                         // time-homogeneous parameters
		                         const std::vector<QuantLib::Real>&                 delta,   // maturity of benchmark rates f(t,t+delta_i) 		
		                         const std::vector<QuantLib::Real>&                 chi,     // mean reversions
		                         const std::vector< std::vector<QuantLib::Real> >&  Gamma,   // (benchmark rate) correlation matrix
		                         // stochastic volatility process parameters
		                         QuantLib::Real                                     theta,   // mean reversion speed
 				                 const QuantLib::RealStochasticProcess::VolEvolv    volEvolv,
		                         const std::vector<QuantLib::Real>&                 procLimit,  // stochastic process limits
								 const bool                                         useSwapRateScaling,  // re-scale alpha and b to match swap dynamics
						   bool permanent) : RealStochasticProcess(properties,permanent) {

        libraryObject_ = boost::shared_ptr<QuantLib::RealStochasticProcess>(
			new QuantLib::RealQuasiGaussianModel( hYTS, d, times, lambda, alpha, b, eta, delta, chi, Gamma, theta, volEvolv, procLimit, useSwapRateScaling ));
    }

	RealQGSwaptionModel::RealQGSwaptionModel(
		                    const boost::shared_ptr<ObjectHandler::ValueObject>&         properties,
						    const boost::shared_ptr<QuantLib::RealQuasiGaussianModel>&   model,
			                const std::vector<QuantLib::Time>&                           floatTimes,    // T[1], ..., T[M]
			                const std::vector<QuantLib::Real>&                           floatWeights,  // u[1], ..., u[M]
			                const std::vector<QuantLib::Time>&                           fixedTimes,    // T[1], ..., T[N]
			                const std::vector<QuantLib::Real>&                           fixedWeights,  // w[1], ..., w[N-1]
			                const std::vector<QuantLib::Time>&                           modelTimes,   // time grid for numerical integration
			                const bool                                                   useExpectedXY, // evaluate E^A [ x(t) ], E^A [ y(t) ] as expansion points
							bool permanent) : RealTDStochVolModel(properties,permanent) {
        libraryObject_ = boost::shared_ptr<QuantLib::RealQGSwaptionModel>(
			new QuantLib::RealQGSwaptionModel( model, floatTimes, floatWeights, fixedTimes, fixedWeights, modelTimes, useExpectedXY ) );
	}

	QuasiGaussianModelCalibrator::QuasiGaussianModelCalibrator(
		                             const boost::shared_ptr<ObjectHandler::ValueObject>&         properties,
			                         boost::shared_ptr<QuantLib::RealQuasiGaussianModel>          model,
			                         boost::shared_ptr<QuantLib::RealMCSimulation>                mcSimulation,
									 std::vector< boost::shared_ptr<QuantLib::Swaption> >         swaptions,
									 std::vector< std::vector< QuantLib::Real > > lambda,
                                     std::vector< std::vector< QuantLib::Real > > b,
                                     std::vector< std::vector< QuantLib::Real > > eta,
									 QuantLib::Real                               lambdaMin,
									 QuantLib::Real                               lambdaMax,
									 QuantLib::Real                               bMin,
									 QuantLib::Real                               bMax,
									 QuantLib::Real                               etaMin,
									 QuantLib::Real                               etaMax,
									 std::vector< QuantLib::Real >                modelTimes,
                                     bool                                         useExpectedXY,
			                         bool permanent) : ObjectHandler::LibraryObject<QuantLib::QuasiGaussianModelCalibrator>(properties,permanent) {
        // we need to re-order swaptions in matrix form
		QL_REQUIRE( lambda.size()>0, "QuasiGaussianModelCalibrator: lambda required." );
		QuantLib::Size nRows = lambda.size();
		QuantLib::Size nCols = lambda[0].size();
		QL_REQUIRE( swaptions.size()==nRows*nCols, "QuasiGaussianModelCalibrator: wrong number of swaptions." );
		std::vector< std::vector< boost::shared_ptr<QuantLib::Swaption> > > swaptionMatrix;
		swaptionMatrix.resize(nRows);
		for (QuantLib::Size i=0; i<nRows; ++i) {
			swaptionMatrix[i].resize(nCols);
			for (QuantLib::Size j=0; j<nCols; ++j) {
				swaptionMatrix[i][j] = swaptions[i*nCols+j];
			}
		}

        libraryObject_ = boost::shared_ptr<QuantLib::QuasiGaussianModelCalibrator>(
			new QuantLib::QuasiGaussianModelCalibrator( model, mcSimulation, swaptionMatrix, lambda, b, eta, lambdaMin, lambdaMax, bMin, bMax, etaMin, etaMax, modelTimes, useExpectedXY ) );
	}

	// model from calibration
	RealQuasiGaussianModel::RealQuasiGaussianModel(
		                         const boost::shared_ptr<ObjectHandler::ValueObject>&              properties, 
			                     const boost::shared_ptr<QuantLib::QuasiGaussianModelCalibrator>&  calibrator,
					             const std::vector< std::vector< bool > >&                         isInput,
			                     const std::vector< std::vector< bool > >&                         isOutput,
								 	// optimization parameters
		                         QuantLib::Real                                                    epsfcn,
								 QuantLib::Real                                                    ftol,
								 QuantLib::Real                                                    xtol,
								 QuantLib::Real                                                    gtol,
		                         QuantLib::Size                                                    maxfev,
			                     bool permanent) : RealStochasticProcess(properties,permanent) {
	    libraryObject_ = boost::shared_ptr<QuantLib::RealStochasticProcess>( calibrator->calibrate(isInput,isOutput,epsfcn,ftol,xtol,gtol,maxfev) );
	}

}
