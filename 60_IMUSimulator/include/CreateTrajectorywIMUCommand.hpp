#pragma once

#include "IMU_std_Lib.hpp"


void IMUCommandForTrajectory(std::string, std::string);
/*
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "GPSWeekSecond.hpp"

#include <Eigen/Dense>

#include "strapdown_ecef.h"
#include "geoparam_wgs84.h"
#include "lib_Frames.h"

#include "Trajectory.h"
#include "IMUSignalGenerator.h"
#include "IMUStore.h"

//Own includes
#include "Trajectory.h"
#include "IMUSignalGenerator.h"
#include "IMUStore.h"
#include "IMUData.h"

#include "..\68_GINAParser\include\TrajectoryParser\TrajectoryData.hpp"

// GINAParser
#include "IMUBase.hpp"
#include "IMUData.hpp"
#include "IMUHeader.hpp"
#include "IMUStream.hpp"
#include "IMUStore.hpp"

#include "TrajectoryBase.hpp"
#include "TrajectoryData.hpp"
#include "TrajectoryHeader.hpp"
#include "TrajectoryStream.hpp"
#include "TrajectoryStore.hpp"


void IMUGeneratorForTrajectory(std::string, std::string);

void setGINAParsers(const GINASimulator::TrajectoryStream&,
					GINASimulator::TrajectoryHeader&,
					const GINASimulator::IMUStream&,
					GINASimulator::IMUHeader&,
					const Eigen::Vector3d&,
					const Eigen::Vector3d&,
					const unsigned int&, const double&,
					const unsigned int&, const double&,
					const double&);

void generatetrajectory(IMUSimulator::IMUSignalGenerator&,
						IMUSimulator::strapdown_ecef&,
						IMUSimulator::PositionData&,
						IMUSimulator::Measure_IMU&,
						Eigen::Vector3d,
						Eigen::Vector3d,
						unsigned int,
						double,
						double);

GINASimulator::TrajectoryData convert2GINAcompatible(IMUSimulator::PositionData&);
GINASimulator::IMUData convert2GINAcompatible(IMUSimulator::Measure_IMU&);

*/