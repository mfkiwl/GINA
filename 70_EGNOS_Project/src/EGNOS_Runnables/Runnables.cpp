#include "Runnables.hpp"


namespace EGNOS 
{
	namespace EGNOS_RUNNABLE
	{
	#define TEC_IN_METER 0.162372
		

		void compareIonexFiles(	std::string ReferenceIonexFileNamewPath,
								std::string TargetIonexFileNamewPath,
								std::string IonexFileNamewPath_Out) {

			gpstk::IonexStore ionoStoreRef, ionoStoreTarget;
			gpstk::IonexHeader ionoHeader1, ionoHeader2;

			ionoStoreRef.loadFile(ReferenceIonexFileNamewPath);
			ionoHeader1 = ionoStoreRef.getHeader(ReferenceIonexFileNamewPath);

			ionoStoreTarget.loadFile(TargetIonexFileNamewPath);
			ionoHeader2 = ionoStoreTarget.getHeader(TargetIonexFileNamewPath);

			gpstk::IonexStream outIonexStoreStream;
			outIonexStoreStream.open(IonexFileNamewPath_Out.c_str(), std::ios::out);

			std::vector<std::string> names = ionoStoreRef.getFileNames();
			outIonexStoreStream << ionoStoreRef.getHeader(names[0]);

			gpstk::IonexStore::IonexMap::iterator it;
			gpstk::CommonTime epoch;

			gpstk::IonexHeader refHeader = ionoStoreRef.getHeader(names[0]);

			int mapID = 0;

			std::vector<gpstk::IonexData> ionexDataVector;

			for (it = ionoStoreRef.inxMaps.begin(); it != ionoStoreRef.inxMaps.end(); it++)	{

				epoch = it->first;		// key

				ionexDataVector = EGNOS_RUNNABLE_UTILITY::createDifferenceDataBlock(refHeader,
																					ionoStoreRef,
																					ionoStoreTarget,
																					epoch,
																					mapID);

				for (size_t index = 0; index < ionexDataVector.size(); index++){

					outIonexStoreStream << ionexDataVector[index];
				}
				
				mapID++;
			}

			outIonexStoreStream.close();
		}

		void processEMS(std::string EDAS_FileNamewPath,
						std::string Output_IonexFileNamewPath,
						std::string Output_IonexFileNamewPathLast,
						std::string Output_IonexFileNamewPath_Detailed,
						EGNOSMapType mapType,
						bool interPolationOn,
						double updateIntervalinSeconds) {


			if (EGNOS_RUNNABLE_UTILITY::checkArgs(	EDAS_FileNamewPath,
													Output_IonexFileNamewPath,
													Output_IonexFileNamewPathLast,
													Output_IonexFileNamewPath_Detailed,
													mapType,
													interPolationOn,
													updateIntervalinSeconds) == false) 
			{
				return;
			}

			EGNOS_EMS_Parser::EGNOS_EMS_Stream EMSStream(EDAS_FileNamewPath.c_str());

			EGNOS_EMS_Parser::EGNOS_EMS_Data EData;

			EGNOS::IonosphericDelayCorrectionsMessageParser IonoGridPointParser;
			EGNOS::IonosphericGridPointMasksMessageParser IonoMaskParser;

			EGNOS::IGPMapStore igpMapStore;

			gpstk::CommonTime CurrentDataTime;
			gpstk::CommonTime LastUpdateTime;


			EGNOS::IGPMap IonoMap;
			EGNOS::VerticalIonoDelayInterpolator egnosInterPol;
			EGNOS::IonexCreator ionexWriter;
			ionexWriter.headerType = mapType; //EGNOS::europe5x5_tec; // europe5x5_tec | europe2_5x2_5_tec |europe1x1_tec
			EGNOS::IGPMediator IgpMediator;

			if (interPolationOn == true)
				ionexWriter.setInterpolator(egnosInterPol);

			bool weHad18 = false;
			bool weHad26 = false;

			int updateIndex = 0;
			while (EMSStream >> EData) {

				try	{
					CurrentDataTime = EData.messageTime;
				}
				catch (...)	{

					cout << "Time conversion failed so we skipped this epoch. Civil Time: " << EData.messageTime.asString() << endl;
					continue;
				}
				

				if (EData.messageId == 18) {

					IonoMaskParser += EData.message;
					//cout << IonoMaskParser << endl;
					weHad18 = true;
				}

				if (EData.messageId == 26) {

					IonoGridPointParser += EData.message;
					//cout << IonoGridPointParser << endl;
					weHad26 = true;
				}

				if (weHad18 || weHad26) {

					bool newData = false;

					IgpMediator.updateTime(CurrentDataTime);
					IgpMediator.setIGPCandidates(IonoGridPointParser.getIonosphericGridPoint());
					IgpMediator.updateIGPCandidate(IonoMaskParser);

					newData = IonoMap.updateMap(IgpMediator);

					
					if (newData == true) {
						
						if (updateIntervalinSeconds > 0) {
						
							if (CurrentDataTime > (LastUpdateTime + updateIntervalinSeconds)) {

								LastUpdateTime = CurrentDataTime;

								igpMapStore.addMap(CurrentDataTime, IonoMap);

								gpstk::CivilTime clock(CurrentDataTime);
								cout << "Time: " << clock.asString() << ": IGP Map was updated and added to IGPMapStore" << endl;
							}
							
						}
						else {
							igpMapStore.addMap(CurrentDataTime, IonoMap);
							gpstk::CivilTime clock(CurrentDataTime);
							cout << "Time: " << clock << ": IGP Map was updated and added to IGPMapStore" << endl;
						}

						if (Output_IonexFileNamewPath_Detailed != "") {
							updateIndex++;
							string ems_out_file_index = EGNOS_RUNNABLE_UTILITY::createStrFileIndex(updateIndex);
							std::string ionexFile_Out_Detailed = Output_IonexFileNamewPath_Detailed;
							ionexFile_Out_Detailed = ionexFile_Out_Detailed.insert(Output_IonexFileNamewPath_Detailed.size() - 4, ems_out_file_index.c_str());

							ionexWriter.setIonexCompatibleMap(IonoMap);
							ionexWriter.writeIGPMap2file(ionexFile_Out_Detailed);
							cout << "IGP Map was written to a standalone ionex file" << endl;
						}
						
					}

					//cout << IonoMap;

					weHad26 = false;
					weHad18 = false;
				}
			}

			EMSStream.close();
			//cout << IonoMap;

			if (Output_IonexFileNamewPathLast != "") {

				ionexWriter.setIonexCompatibleMap(IonoMap);
				ionexWriter.writeIGPMap2file(Output_IonexFileNamewPathLast);
			}

			if (Output_IonexFileNamewPath != "") {
				ionexWriter.setIonexCompatibleMap(igpMapStore);
				ionexWriter.writeIGPMap2file(Output_IonexFileNamewPath);
			}

			IGPMap2IonexData ionexConverter;

			gpstk::IonexStore ionexStore = ionexConverter.convert(igpMapStore);

			cout << "Init Time: " << ionexStore.getInitialTime() << endl;
			cout << "Final Time: " << ionexStore.getFinalTime() << endl;

			gpstk::Position RX;
			RX.setGeocentric(32.3, 15.4, 0);

			try
			{
				cout << endl << endl;
				gpstk::Triple rtv = ionexStore.getIonexValue(ionexStore.getInitialTime() + 1000, RX, 1);
				cout << rtv << endl;

			}
			catch (...)
			{
				std::cout << "We could not get TEC and RMS information from the IonexStore" << std::endl;
			}

			cout << endl << endl;

			//ionexStore.dump();

			return;
		}
	};
		
	namespace EGNOS_RUNNABLE_UTILITY
	{
		bool checkArgs(std::string EDAS_FileNamewPath,
			std::string Output_IonexFileNamewPath,
			std::string Output_IonexFileNamewPathLast,
			std::string Output_IonexFileNamewPath_Detailed,
			EGNOS::EGNOSMapType mapType,
			bool interPolationOn,
			unsigned int updateIntervalinSeconds) {

			if (EDAS_FileNamewPath == "") {
				std::cout << "First give me an EMS file" << std::endl;
				return false;
			}

			if (interPolationOn == true) {
				std::cout << "Enabled: Spatial interpolation" << std::endl;
			}
			else {
				std::cout << "Disabled: Spatial interpolation" << std::endl;
			}

			if (updateIntervalinSeconds == 0) {
				std::cout << "IGPMapStore is updated every time when we have an update from EGNOS" << std::endl;
			}
			else {
				std::cout << "IGPMapStore is updated in fix intervals. Interval: "<< updateIntervalinSeconds << " s" << std::endl;
			}

			std::cout << "Map Type: " << EGNOSMapType2String(mapType) << std::endl;

			if (Output_IonexFileNamewPath_Detailed == "") {
				std::cout << "Enabled:  Writing out each newly updated IGPMap in separate files during EMS file process." << endl;
			}
			else {
				std::cout << "Disabled: Writing out each newly updated IGPMap in separate files during EMS file process." << std::endl;
			}

			if (Output_IonexFileNamewPathLast != "") {
				std::cout << "Enabled: Last IGPMap will be written to a separate ionex file." << std::endl;
			}
			else {
				std::cout << "Disabled: Last IGPMap will be written to a separate ionex file." << std::endl;
			}

			if (Output_IonexFileNamewPath != "") {
				std::cout << "Enabled: All IGPMap will be written to the same ionex file." << std::endl;
			}
			else {
				std::cout << "Disabled: All IGPMap will be written to the same ionex file." << std::endl;
			}

			return true;
		}

		string createStrFileIndex(int index) {

			index = abs(index);

			string ems_out_file_index = std::to_string(index);

			if (index < 10) {
				ems_out_file_index = "0000" + ems_out_file_index;
			}
			else if (index >= 10 && index < 100) {
				ems_out_file_index = "000" + ems_out_file_index;
			}
			else if (index >= 100 && index < 1000) {
				ems_out_file_index = "00" + ems_out_file_index;
			}
			else if (index >= 1000 && index < 10000) {
				ems_out_file_index = "0" + ems_out_file_index;
			}
			else if (index >= 10000 && index < 100000) {
				// Do nothing
			}
			else {
				cout << "file index is to big" << endl;
			}

			return ems_out_file_index;
		}

		int calcDim(int lat1, int lat2, double dlat) {

			if (dlat == 0) {
				return 1;
			}
			else {
				return int(abs((int(lat2) - int(lat1)) / dlat) + 1);
			}
		}

		std::vector<gpstk::IonexData> createDifferenceDataBlock(	gpstk::IonexHeader header,
																	gpstk::IonexStore refreceStore,
																	gpstk::IonexStore targetStore,
																	gpstk::CommonTime &epoch,
																	int mapID) {
			
			std::vector<gpstk::IonexData> rtv;
			gpstk::IonexData ionexDataTEC;
			gpstk::IonexData ionexDataRMS;

			double lat1, lat2, dlat;
			double lon1, lon2, dlon;
			double hgt1, hgt2, dhgt;
			int dimlat, dimlon, dimhgt;

			lat1 = header.lat[0];
			lat2 = header.lat[1];
			dlat = header.lat[2];

			lon1 = header.lon[0];
			lon2 = header.lon[1];
			dlon = header.lon[2];

			hgt1 = header.hgt[0];
			hgt2 = header.hgt[1];
			dhgt = header.hgt[2];

			dimlat = calcDim(lat1, lat2, dlat);
			dimlon = calcDim(lon1, lon2, dlon);
			dimhgt = calcDim(hgt1, hgt2, dhgt);

			double numberOfValues = dimlat * dimlon;

			gpstk::Vector<double> values(numberOfValues);
			int counter = 0;

			double currLat = lat1;
			double currLon = lon1;
			double TECorRMS;

			gpstk::CommonTime initTime = refreceStore.getInitialTime();
			gpstk::CommonTime finalTime = refreceStore.getFinalTime();

			double diffTEC;
			double diffRMS;

			gpstk::Triple TECRMS1, TECRMS2;

			gpstk::Vector<double> valuesTEC(numberOfValues);
			gpstk::Vector<double> valuesRMS(numberOfValues);
			gpstk::Position RX;

			while (counter < numberOfValues)
			{

				RX.setGeocentric(currLat, currLon, 0);

				try
				{
					TECRMS1 = refreceStore.getIonexValue(epoch, RX, 1);
					TECRMS2 = targetStore.getIonexValue(epoch, RX, 1);

					diffTEC = TECRMS1[0] - TECRMS2[0];
					diffRMS = TECRMS1[1] - TECRMS2[1];

					//if (diffTEC < 0){
					//	cout << endl;//diffTEC = 999.9 - diffTEC;
					//}

					//if (diffRMS < 0) {
					//	cout << endl;
					//	//diffRMS = 999.9 - diffRMS;
					//}

					diffTEC = diffTEC / TEC_IN_METER;
					diffRMS = diffRMS / TEC_IN_METER;
					
					valuesTEC(counter) = diffTEC;
					valuesRMS(counter) = diffRMS;
				}
				catch (...)
				{
					valuesTEC(counter) = 999.9;
					valuesRMS(counter) = 999.9;
				}
				

				

				if (abs(currLon - lon2) < dlon) {
					currLon = lon1 - dlon;
					currLat += dlat;
				}
				currLon += dlon;
				counter++;
			}

			ionexDataTEC.data = valuesTEC;
			ionexDataTEC.mapID = mapID;

			ionexDataRMS.data = valuesRMS;
			ionexDataRMS.mapID = mapID;

			ionexDataTEC.dim[0] = dimlat;
			ionexDataTEC.dim[1] = dimlon;
			ionexDataTEC.dim[2] = dimhgt;

			ionexDataRMS.dim[0] = dimlat;
			ionexDataRMS.dim[1] = dimlon;
			ionexDataRMS.dim[2] = dimhgt;

			ionexDataTEC.exponent = -1;
			ionexDataTEC.lat[0] = lat1;
			ionexDataTEC.lat[1] = lat2;
			ionexDataTEC.lat[2] = dlat;

			ionexDataRMS.exponent = -1;
			ionexDataRMS.lat[0] = lat1;
			ionexDataRMS.lat[1] = lat2;
			ionexDataRMS.lat[2] = dlat;

			ionexDataTEC.lon[0] = lon1;
			ionexDataTEC.lon[1] = lon2;
			ionexDataTEC.lon[2] = dlon;

			ionexDataRMS.lon[0] = lon1;
			ionexDataRMS.lon[1] = lon2;
			ionexDataRMS.lon[2] = dlon;

			ionexDataTEC.hgt[0] = hgt1;
			ionexDataTEC.hgt[1] = hgt2;
			ionexDataTEC.hgt[2] = dhgt;

			ionexDataRMS.hgt[0] = hgt1;
			ionexDataRMS.hgt[1] = hgt2;
			ionexDataRMS.hgt[2] = dhgt;

			ionexDataTEC.valid = true;
			ionexDataRMS.valid = true;

			ionexDataTEC.type.type = "TEC";
			ionexDataRMS.type.type = "RMS";

			ionexDataTEC.type.units = "10e-1 [TEC]";
			ionexDataRMS.type.units = "10e-1 [TEC]";

			ionexDataTEC.type.description = "Total Electron Content Difference map";
			ionexDataRMS.type.description = "Total Electron Content Difference map";

			ionexDataTEC.time = epoch;
			ionexDataRMS.time = epoch;

			rtv.push_back(ionexDataTEC);
			rtv.push_back(ionexDataRMS);

			return rtv;
		}
		std::string EGNOSMapType2String(EGNOS::EGNOSMapType mapType) {
		
			std::string mapTypeStr;
			switch (mapType)
			{
			case EGNOS::EGNOSMapType::world5x5_tec:
				mapTypeStr = "world5x5_tec";
				break;
			case EGNOS::EGNOSMapType::world5x5_meter:
				mapTypeStr = "world5x5_meter";
				break;
			case EGNOS::EGNOSMapType::world2_5x2_5_tec:
				mapTypeStr = "world2_5x2_5_tec";
				break;
			case EGNOS::EGNOSMapType::world2_5x2_5_meter:
				mapTypeStr = "world2_5x2_5_meter";
				break;
			case EGNOS::EGNOSMapType::world1x1_tec:
				mapTypeStr = "world1x1_tec";
				break;
			case EGNOS::EGNOSMapType::world1x1_meter:
				mapTypeStr = "world1x1_meter";
				break;
			case EGNOS::EGNOSMapType::europe5x5_tec:
				mapTypeStr = "europe5x5_tec";
				break;
			case EGNOS::EGNOSMapType::europe5x5_meter:
				mapTypeStr = "europe5x5_meter";
				break;
			case EGNOS::EGNOSMapType::europe2_5x2_5_tec:
				mapTypeStr = "europe2_5x2_5_tec";
				break;
			case EGNOS::EGNOSMapType::europe2_5x2_5_meter:
				mapTypeStr = "europe2_5x2_5_meter";
				break;
			case EGNOS::EGNOSMapType::europe1x1_tec:
				mapTypeStr = "europe1x1_tec";
				break;
			case EGNOS::EGNOSMapType::europe1x1_meter:
				mapTypeStr = "europe1x1_meter";
				break;
			default:
				mapTypeStr = "Unkown MapType";
				break;
			}

			return mapTypeStr;
		}
	};

	
};