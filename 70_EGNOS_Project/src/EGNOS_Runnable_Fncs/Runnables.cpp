#include "Runnables.hpp"


namespace EGNOS 
{
	namespace EGNOS_RUNNABLE 
	{
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
	
	
		void processEMS(	std::string EDAS_FileNamewPath,
						std::string Output_IonexFileNamewPath,
						std::string Output_IonexFileNamewPath_Detailed,
						EGNOSMapType mapType) {


			//std::string EDAS_FileNamewPath = ROOT "\\70_EGNOS_Project\\files\\EMS_124_2009_01_06_17\\h17.ems";

			EGNOS_EMS_Parser::EGNOS_EMS_Stream exampleStreamIn(EDAS_FileNamewPath.c_str());

			EGNOS_EMS_Parser::EGNOS_EMS_Data EData;

			EGNOS::IonosphericDelayCorrectionsMessageParser IonoGridPointParser;
			EGNOS::IonosphericGridPointMasksMessageParser IonoMaskParser;

			gpstk::CommonTime CurrentDataTime;

			EGNOS::IGPMap IonoMap;
			EGNOS::VerticalIonoDelayInterpolator egnosInterPol;
			EGNOS::IonexCreator ionexWriter;
			ionexWriter.headerType = mapType; //EGNOS::europe5x5_tec; // europe5x5_tec | europe2_5x2_5_tec |europe1x1_tec
			EGNOS::IGPMediator IgpMediator;

			//ionexWriter.setInterpolator(egnosInterPol);

			bool weHad18 = false;
			bool weHad26 = false;

			int updateIndex = 0;
			while (exampleStreamIn >> EData) {

				CurrentDataTime = EData.messageTime;

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

					if (Output_IonexFileNamewPath_Detailed != "" ) {
						if (newData == true) {
							updateIndex++;
							string ems_out_file_index = createStrFileIndex(updateIndex);
							//std::string ionexFile_Out = ROOT "\\70_EGNOS_Project\\files\\EMS_124_2009_01_06_17\\Ionex_from_h17_Europe\\h17ems_ionex_Europe_" + ems_out_file_index + ".18i";
							std::string ionexFile_Out_Detailed = Output_IonexFileNamewPath_Detailed + ems_out_file_index;

							ionexWriter.setIonexData(IonoMap);
							ionexWriter.writeIGPMap2file(ionexFile_Out_Detailed);
							cout << "IGP Map updated and file created" << endl;
						}
					}
					
					//cout << IonoMap;

					weHad26 = false;
					weHad18 = false;
				}
			}

			//cout << IonoMap;
			//std::string ionexFile_Out = ROOT "\\70_EGNOS_Project\\files\\EMS_124_2009_01_06_17\\h17ems_ionex_Europe_5x5_IGPMap.18i";
			//std::string ionexFile_Out = ROOT "\\70_EGNOS_Project\\files\\EMS_124_2009_01_06_17\\h17ems_ionex_Europe_2.5x2.5_IGPMap.18i";
			//std::string ionexFile_Out = ROOT "\\70_EGNOS_Project\\files\\EMS_124_2009_01_06_17\\h17ems_ionex_Europe_5x5_InterPol.18i";
			//std::string ionexFile_Out = ROOT "\\70_EGNOS_Project\\files\\EMS_124_2009_01_06_17\\h17ems_ionex_Europe_2.5x2.5_InterPol.18i";
			//std::string ionexFile_Out = ROOT "\\70_EGNOS_Project\\files\\EMS_124_2009_01_06_17\\h17ems_ionex_Europe_1x1_InterPol.18i";

			if (Output_IonexFileNamewPath != "") {
				ionexWriter.setIonexData(IonoMap);
				ionexWriter.writeIGPMap2file(Output_IonexFileNamewPath);
			}
			

			exampleStreamIn.close();

			return;
		}


	};
};