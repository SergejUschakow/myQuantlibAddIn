/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2010 Sebastian Schlenkrich

 */

#ifndef qla_templatequasigaussianmodels_hpp
#define qla_templatequasigaussianmodels_hpp

#include <qlo/baseinstruments.hpp>
#include <qlo/termstructures.hpp>
#include <qlo/pricingengines.hpp>

#include <qlo/templatehestonmodel.hpp>

#include <ql/types.hpp>
#include <ql/experimental/template/qgaussian/quasigaussianmodels.hpp>
#include <ql/experimental/template/qgaussian/qgaussiancalibrator.hpp>




namespace QuantLib {
    template <class T>
    class Handle;

	class TemplateModel;
}

namespace QuantLibAddin {

    // OH_LIB_CLASS(RealStochasticProcess, QuantLib::RealStochasticProcess);

    class RealQuasiGaussianModel : public RealStochasticProcess {
      public:
		  RealQuasiGaussianModel(const boost::shared_ptr<ObjectHandler::ValueObject>& properties, bool permanent) : RealStochasticProcess(properties,permanent) {}
          RealQuasiGaussianModel(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
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
							     bool permanent);

		  // model from calibration
		  RealQuasiGaussianModel(const boost::shared_ptr<ObjectHandler::ValueObject>&              properties, 
			                     const boost::shared_ptr<QuantLib::QuasiGaussianModelCalibrator>&  calibrator,
					             const std::vector< std::vector< bool > >&                         isInput,
			                     const std::vector< std::vector< bool > >&                         isOutput,
								 	// optimization parameters
		                         QuantLib::Real                                                    epsfcn,
								 QuantLib::Real                                                    ftol,
								 QuantLib::Real                                                    xtol,
								 QuantLib::Real                                                    gtol,
		                         QuantLib::Size                                                    maxfev,
			                     bool permanent);

    };

	class RealQGSwaptionModel : public RealTDStochVolModel {
	public:
		RealQGSwaptionModel(const boost::shared_ptr<ObjectHandler::ValueObject>&         properties,
						    const boost::shared_ptr<QuantLib::RealQuasiGaussianModel>&   model,
			                const std::vector<QuantLib::Time>&                           floatTimes,    // T[1], ..., T[M]
			                const std::vector<QuantLib::Real>&                           floatWeights,  // u[1], ..., u[M]
			                const std::vector<QuantLib::Time>&                           fixedTimes,    // T[1], ..., T[N]
			                const std::vector<QuantLib::Real>&                           fixedWeights,  // w[1], ..., w[N-1]
			                const std::vector<QuantLib::Time>&                           modelTimes,    // time grid for numerical integration
			                const bool                                                   useExpectedXY, // evaluate E^A [ x(t) ], E^A [ y(t) ] as expansion points
							bool permanent);
	};


	class QuasiGaussianModelCalibrator : public ObjectHandler::LibraryObject<QuantLib::QuasiGaussianModelCalibrator> {
	public:
		QuasiGaussianModelCalibrator(const boost::shared_ptr<ObjectHandler::ValueObject>&         properties,
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
			                         bool permanent);
	};

}  // namespace QuantLibAddin

#endif  // quasigaussianmodels

