#pragma once
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include "IGPMap.hpp"

#include "IonexStream.hpp"
#include "IonexHeader.hpp"
#include "IonexData.hpp"
#include "CivilTime.hpp"

namespace EGNOS {

	typedef enum {
		world5x5_tec,		// TODO: add good descriptions
		world5x5_meter,
		world2_5x2_5_tec,
		world2_5x2_5_meter,
		world1x1_tec,
		world1x1_meter,
		europe5x5_tec,
		europe5x5_meter,
		europe2_5x2_5_tec,
		europe2_5x2_5_meter,
		europe1x1_tec,
		europe1x1_meter
	}EGNOSMapType;

	typedef struct {
		double rlat;
		double rlon;
		double rheight;
	}SlantIonoDelay_RoverPos;

	typedef struct {
		double azimuthOfSatId;
		double elevationOfSatId;
	}SlantIonoDelay_SatVisibility;

	typedef struct {
		SlantIonoDelay_RoverPos RoverPos;
		SlantIonoDelay_SatVisibility SatVisibility;
	}SlantIonoDelay_Input;

	class SlantIonoDelay
	{
	public:

		static const double SlantIonoDelay::Re;
		static const double SlantIonoDelay::hI;

		
		double getSlantFactor(SlantIonoDelay_Input data);

	private:

		void setRoverPosition(double lat, double lon, double height);
		void setazimuthOfSatId(double az, double el);

		void calculatePP(void);
		double calculateSlantFactor(void);

		double rlat;
		double rlon;
		double rheight;

		double azimuthOfSatId;
		double elevationOfSatId;
		double ppLat, ppLon;
	};

	class VerticalIonoDelayInterpolator: public IonexCompatibleInterPolator
	{

	public:
		typedef struct VerticesOfSquare {
			IonosphericGridPoint first;
			IonosphericGridPoint second;
			IonosphericGridPoint third;
			IonosphericGridPoint fourth;
		};

		VerticalIonoDelayInterpolator(void) {};
		VerticalIonoDelayInterpolator(VerticalIonoDelayInterpolator* original);
		~VerticalIonoDelayInterpolator(void) {};

		IonCorrandVar VerticalIonoDelayInterpolator::interpolate(IGPMapBase& Map, IonosphericGridPoint& newPP);
		IonosphericGridPoint getHorizontallyInterpolatedVertices(IGPMapBase& Map, double lat, double lon, double increment);

		double getTEC(IonexCompatibleMap *Map, gpstk::CommonTime epoch, double lat, double lon);
		double getRMS(IonexCompatibleMap *Map, gpstk::CommonTime epoch, double lat, double lon);
		IonexCompatibleInterPolator* clone() const { return new VerticalIonoDelayInterpolator(*this); };
		
	private:

		// With this step will the program search for available igps.
		double incrementOfSearchWindowforHorizontalInterPolator = 5;

		IonosphericGridPoint ionoPP;

		IonosphericGridPoint getIGP(IGPMapBase &Map, double lat, double lon);

		
		IonCorrandVar grid5x5Interpolator(IGPMapBase& Map);
		IonCorrandVar grid5x10Interpolator(IGPMapBase& Map);
		IonCorrandVar grid10x10Interpolator(IGPMapBase& Map);
		IonCorrandVar grid10x10InterpolatorwHorizontalInterpolation(IGPMapBase& Map);
		
		void VerticalIonoDelayInterpolator::getPolarVertices(VerticesOfSquare &table, IGPMapBase &Map);
		void getVerticesOf5x5Square(VerticesOfSquare& table, IGPMapBase& Map);
		void getVerticesOf5x10Square(VerticesOfSquare& table, IGPMapBase& Map);
		void getVerticesOf10x10Square(VerticesOfSquare& table, IGPMapBase& Map);
		void getVerticesOf10x10SquarewHorizontalInterpolation(VerticesOfSquare& table, IGPMapBase& Map);
		IonosphericGridPoint getHorizontallyNearestIGP(IGPMapBase& Map, double originalLat, double originalLon, double lat, double lon, double increment);

		void setPP(IonosphericGridPoint newPP);
		double interpolation4point(double xpp, double ypp,
			double ionoDelay1,
			double ionoDelay2,
			double ionoDelay3,
			double ionoDelay4);
		double interpolation3point(double xpp, double ypp,
			double ionoDelay2,
			double ionoDelay3,
			double ionoDelay4);
		
		IonCorrandVar polarInterpolator(IGPMapBase& Map);
		IonCorrandVar Interpolator(double latDistance, double lonDistance, VerticesOfSquare table);	

		void calculate_xpp_and_ypp(	double &xpp,	double &ypp,
									double &lat1,	double &lat2,
									double &lon1,	double &lon2);

		void getNearestLatLot(double &lat1, double &lat2, double &lon1, double &lon2);
		void restrictLong(double *indegree);
		double restrictLong(double indegree);
		void restrictLonginDegree(double &indegree);
		double absDistanceOfLongitude(double lon1, double lon2);
		int closestNumberFromLow(int n, int m);
		int closestNumberFromHigh(int n, int m);
		int secondClosestNumberFromLow(int n, int m);
		int secondClosestNumberFromHigh(int n, int m);
	};

	class IonexCreator
	{
		static const int IonexCreator::maxValue;
		static const int IonexCreator::invalidValue;

		typedef enum{
			TEC,
			RMS
		}dataType;

		typedef enum {
			meter,
			tec
		}unitType;

		public:
			IonexCreator(void) {};
			~IonexCreator(void);

			void setIonexData(IonexCompatibleMap &Ionex);
			void setInterpolator(IonexCompatibleInterPolator &interPol);
			bool writeIGPMap2file(std::string newIonexFile);

			EGNOSMapType headerType = EGNOSMapType::world5x5_meter;

		private:
	
			unitType unit = unitType::meter;
			bool isInterPolatorSet = false;

			std::string ionexFile;
			gpstk::IonexHeader header;
			gpstk::IonexStream strm;
			std::vector<gpstk::CommonTime> epochs;
			IonexCompatibleMap *ionoData;
			IonexCompatibleInterPolator* interPol;

			bool getMapEpochs(void);
			void createHeader(void);
			void createHeader_World(double gridSize);
			void createHeader_Europe(double gridSize);
			void createHeaderBase(void);

			gpstk::IonexData createDataBlock(gpstk::CommonTime currentEpoch, int mapID, dataType type);
			double getData(gpstk::CommonTime currentEpoch, double currLat, double currLon, dataType type);

			void writeHeader(gpstk::IonexHeader &header);
			void writeData(gpstk::IonexData &data);

			std::string getCurrentTimeinStr(void);
			void openFile(void);
			void closeFile(void);

			int calcDim(int lat1, int lat2, double dlat);
			double calculateIntervalinSec(gpstk::CommonTime firstEpoch, gpstk::CommonTime secondEpoch);
			std::string typeString(dataType type);

			double maxHeight = 350;
			double minHeight = 350;
			double heightDistance = 0;

			double maxLat = 90;
			double minLat = -90;
			double latDistance = -5;

			double maxLon = 180;
			double minLon = -180;
			double lonDistance = 5;

			double baseRadius = 6371.4;
			double mapDims = 2;

			double exponent = -1;
			double version = 1.0;
			std::string fileType{ "IONOSPHERE MAPS" };
			std::string system{ "GPS" };
			std::string fileProgram{ "GINA 1.0" };
			std::string fileAgency{ "GINA Project" };
			std::vector<std::string> description = { { "Infrastructure: Robert Bosch Kft., Budapest" }	,
			{ "Creator: Balazs Lupsic" }						,
			{ "Contact adress: balazs.lupsic@hu.bosch.com" }	,
			{ "Contact adress: balazs.lupsic@gmail.com" } };

			std::string comment_part1 = std::string("TEC values in ");
			std::string comment_part2 = std::string("9999, if no value available");
			std::string mappingFunction = std::string("TBD");
			double elevation = 0;
			std::string observablesUsed = std::string("TEC is obtained from EGNOS");
			int numStations = 0;
			int numSVs = 0;

	};
};