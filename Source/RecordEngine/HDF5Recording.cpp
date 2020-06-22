/*
 ------------------------------------------------------------------

 This file is part of the Open Ephys GUI
 Copyright (C) 2014 Florian Franzen

 ------------------------------------------------------------------

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "HDF5Recording.h"
#define MAX_BUFFER_SIZE 40960
#define CHANNEL_TIMESTAMP_PREALLOC_SIZE 128
#define CHANNEL_TIMESTAMP_MIN_WRITE	32
#define TIMESTAMP_EACH_NSAMPLES 1024

HDF5Recording::HDF5Recording() : processorIndex(-1), bufferSize(MAX_BUFFER_SIZE), hasAcquired(false)
{
    //timestamp = 0;
    scaledBuffer.malloc(MAX_BUFFER_SIZE);
    intBuffer.malloc(MAX_BUFFER_SIZE);
}

HDF5Recording::~HDF5Recording()
{	
}

String HDF5Recording::getEngineID() const
{
    return "KWIK";
}

// void HDF5Recording::updateTimeStamp(int64 timestamp)
// {
//     this->timestamp = timestamp;
// }

void HDF5Recording::registerProcessor(const GenericProcessor* proc)
{
    KWIKRecordingInfo* info = new KWIKRecordingInfo();
    info->sample_rate = proc->getSampleRate();
    info->bit_depth = 16;
    info->multiSample = false;
    infoArray.add (info);

    fileArray.add (new KWDFile());
    bitVoltsArray.add (new Array<float>);
    sampleRatesArray.add (new Array<float>);
    channelsPerProcessor.add (0);
    processorIndex++;
}

void HDF5Recording::resetChannels()
{
	scaledBuffer.malloc(MAX_BUFFER_SIZE);
	intBuffer.malloc(MAX_BUFFER_SIZE);
	bufferSize = MAX_BUFFER_SIZE;
    processorIndex = -1;
    fileArray.clear();
	channelsPerProcessor.clear();
    bitVoltsArray.clear();
    sampleRatesArray.clear();
    processorMap.clear();
    infoArray.clear();
	recordedChanToKWDChan.clear();
	channelLeftOverSamples.clear();
	channelTimestampArray.clear();
    if (spikesFile)
        spikesFile->resetChannels();
}

void HDF5Recording::addDataChannel(int index,const DataChannel* chan)
{
    processorMap.add(processorIndex);
}

void HDF5Recording::openFiles(File rootFolder, int experimentNumber, int recordingNumber)
{
    String basepath = rootFolder.getFullPathName() + rootFolder.separatorString + "experiment" + String(experimentNumber);
    //KWE file
    eventFile->initFile(basepath);
    eventFile->open();

    //KWX file
    spikesFile->initFile(basepath);
    spikesFile->open();
    spikesFile->startNewRecording(recordingNumber);

    //Let's just put the first processor (usually the source node) on the KWIK for now
    infoArray[0]->name = String("Open Ephys Recording #") + String(recordingNumber);

   /* if (hasAcquired)
        infoArray[0]->start_time = (*timestamps)[getChannel(0)->sourceNodeId]; //(*timestamps).begin()->first;
    else
        infoArray[0]->start_time = 0;*/
	infoArray[0]->start_time = getTimestamp(0);

    infoArray[0]->start_sample = 0;
    eventFile->startNewRecording(recordingNumber,infoArray[0]);

    //KWD files
	recordedChanToKWDChan.clear();
	Array<int> processorRecPos;
	processorRecPos.insertMultiple(0, 0, fileArray.size());
	for (int i = 0; i < getNumRecordedChannels(); i++)
	{
		int index = processorMap[getRealChannel(i)];
		if (!fileArray[index]->isOpen())
		{
			fileArray[index]->initFile(getDataChannel(getRealChannel(i))->getCurrentNodeID(), basepath);
			infoArray[index]->start_time = getTimestamp(i);
		}

		channelsPerProcessor.set(index, channelsPerProcessor[index] + 1);
		bitVoltsArray[index]->add(getDataChannel(getRealChannel(i))->getBitVolts());
		sampleRatesArray[index]->add(getDataChannel(getRealChannel(i))->getSampleRate());
		if (getDataChannel(getRealChannel(i))->getSampleRate() != infoArray[index]->sample_rate)
		{
			infoArray[index]->multiSample = true;
		}
		int procPos = processorRecPos[index];
		recordedChanToKWDChan.add(procPos);
		processorRecPos.set(index, procPos+1);
		channelTimestampArray.add(new Array<int64>);
		channelTimestampArray.getLast()->ensureStorageAllocated(CHANNEL_TIMESTAMP_PREALLOC_SIZE);
		channelLeftOverSamples.add(0);
	} 

    for (int i = 0; i < fileArray.size(); i++)
    {
		if ((!fileArray[i]->isOpen()) && (fileArray[i]->isReadyToOpen()))
		{
			fileArray[i]->open(channelsPerProcessor[i]);
		}
        if (fileArray[i]->isOpen())
        {
           // File f(fileArray[i]->getFileName());
           // eventFile->addKwdFile(f.getFileName());

            infoArray[i]->name = String("Open Ephys Recording #") + String(recordingNumber);
            //           infoArray[i]->start_time = timestamp;
            infoArray[i]->start_sample = 0;
            infoArray[i]->bitVolts.clear();
            infoArray[i]->bitVolts.addArray(*bitVoltsArray[i]);
            infoArray[i]->channelSampleRates.clear();
            infoArray[i]->channelSampleRates.addArray(*sampleRatesArray[i]);
            fileArray[i]->startNewRecording(recordingNumber,bitVoltsArray[i]->size(),infoArray[i]);
        }
    }

    hasAcquired = true;
}

void HDF5Recording::closeFiles()
{
    eventFile->stopRecording();
    eventFile->close();
    spikesFile->stopRecording();
    spikesFile->close();
    for (int i = 0; i < fileArray.size(); i++)
    {
        if (fileArray[i]->isOpen())
        {
			std::cout << "Closed file " << i << std::endl;
            fileArray[i]->stopRecording();
            fileArray[i]->close();
            bitVoltsArray[i]->clear();
			sampleRatesArray[i]->clear();
        }
		channelsPerProcessor.set(i, 0);
    }
	recordedChanToKWDChan.clear();
	channelTimestampArray.clear();
	channelLeftOverSamples.clear();
	scaledBuffer.malloc(MAX_BUFFER_SIZE);
	intBuffer.malloc(MAX_BUFFER_SIZE);
	bufferSize = MAX_BUFFER_SIZE;
}

void HDF5Recording::writeData(int writeChannel, int realChannel, const float* buffer, int size)
{
	if (size > bufferSize) //Shouldn't happen, and if it happens it'll be slow, but better this than crashing. Will be reset on flie close and reset.
	{
		std::cerr << "Write buffer overrun, resizing to" << size << std::endl;
		bufferSize = size;
		scaledBuffer.malloc(size);
		intBuffer.malloc(size);
	}
	double multFactor = 1 / (float(0x7fff) * getDataChannel(realChannel)->getBitVolts());
	int index = processorMap[realChannel]; //CHECK
	FloatVectorOperations::copyWithMultiply(scaledBuffer.getData(), buffer, multFactor, size);
	AudioDataConverters::convertFloatToInt16LE(scaledBuffer.getData(), intBuffer.getData(), size);
	fileArray[index]->writeRowData(intBuffer.getData(), size, recordedChanToKWDChan[writeChannel]);

	int sampleOffset = channelLeftOverSamples[writeChannel];
	int blockStart = sampleOffset;
	int64 currentTS = getTimestamp(writeChannel);

	if (sampleOffset > 0)
	{
		currentTS += TIMESTAMP_EACH_NSAMPLES - sampleOffset;
		blockStart += TIMESTAMP_EACH_NSAMPLES - sampleOffset;
	}
	
	for (int i = 0; i < size; i += TIMESTAMP_EACH_NSAMPLES)
	{
		if ((blockStart + i) < (sampleOffset + size))
		{
			channelTimestampArray[writeChannel]->add(currentTS);
			currentTS += TIMESTAMP_EACH_NSAMPLES;
		}
	}
	channelLeftOverSamples.set(writeChannel, (size + sampleOffset) % TIMESTAMP_EACH_NSAMPLES);
}

void HDF5Recording::endChannelBlock(bool lastBlock)
{
	int nCh = channelTimestampArray.size();
	for (int ch = 0; ch < nCh; ++ch)
	{
		int tsSize = channelTimestampArray[ch]->size();
		if ((tsSize > 0) && ((tsSize > CHANNEL_TIMESTAMP_MIN_WRITE) || lastBlock))
		{
			int realChan = getRealChannel(ch);
			int index = processorMap[realChan]; //CHECK
			fileArray[index]->writeTimestamps(channelTimestampArray[ch]->getRawDataPointer(), tsSize, recordedChanToKWDChan[ch]);
			channelTimestampArray[ch]->clearQuick();
		}
	}
}

void HDF5Recording::writeEvent(int eventChannel, const MidiMessage& event)
{
	if (Event::getEventType(event) == EventChannel::TTL)
	{
		TTLEventPtr ttl = TTLEvent::deserializeFromMessage(event, getEventChannel(eventChannel));
		if (ttl == nullptr) return;
		uint8 channel = ttl->getChannel();
		eventFile->writeEvent(0, (ttl->getState() ? 1 : 0), ttl->getSourceID(), &channel, ttl->getTimestamp());
	}
	else if (Event::getEventType(event) == EventChannel::TEXT)
	{
		TextEventPtr text = TextEvent::deserializeFromMessage(event, getEventChannel(eventChannel));
		if (text == nullptr) return;
		String textMsg = text->getText();
		eventFile->writeEvent(1, 0, text->getSourceID(), textMsg.toUTF8().getAddress(), text->getTimestamp());
	}
}

void HDF5Recording::writeTimestampSyncText(uint16 sourceID, uint16 sourceIdx, int64 timestamp, float, String text)
{
	eventFile->writeEvent(1, 0xFF, sourceID, text.toUTF8().getAddress(), timestamp);
}

void HDF5Recording::addSpikeElectrode(int index, const SpikeChannel* elec)
{
    spikesFile->addChannelGroup(elec->getNumChannels());
}
void HDF5Recording::writeSpike(int electrodeIndex, const SpikeEvent* spike)
{
	const SpikeChannel* spikeInfo = getSpikeChannel(electrodeIndex);
	Array<float> bitVolts;
	for (int i = 0; i < spikeInfo->getNumChannels(); i++)
		bitVolts.add(spikeInfo->getChannelBitVolts(i));
    spikesFile->writeSpike(electrodeIndex,spikeInfo->getTotalSamples(),spike->getDataPointer(), bitVolts, spike->getTimestamp());
}

void HDF5Recording::startAcquisition()
{
    eventFile = new KWEFile();
    eventFile->addEventType("TTL",HDF5FileBase::BaseDataType::U8,"event_channels");
	eventFile->addEventType("Messages", HDF5FileBase::BaseDataType::DSTR, "Text");
    spikesFile = new KWXFile();
}

RecordEngineManager* HDF5Recording::getEngineManager()
{
    RecordEngineManager* man = new RecordEngineManager("KWIK","Kwik",&(engineFactory<HDF5Recording>));
    return man;
}
