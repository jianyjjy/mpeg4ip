/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4common.h"

MP4Track::MP4Track(MP4File* pFile, MP4Atom* pTrakAtom) 
{
	m_pFile = pFile;
	m_pTrakAtom = pTrakAtom;

	m_writeSampleId = 1;
	m_fixedSampleDuration = 0;
	m_pChunkBuffer = NULL;
	m_chunkBufferSize = 0;
	m_chunkSamples = 0;
	m_chunkDuration = 0;

	m_samplesPerChunk = 0;
	m_durationPerChunk = 0;

	bool success = true;

	MP4Integer32Property* pTrackIdProperty;
	success &= m_pTrakAtom->FindProperty(
		"trak.tkhd.trackId",
		(MP4Property**)&pTrackIdProperty);
	if (success) {
		m_trackId = pTrackIdProperty->GetValue();
	}

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.mdhd.timeScale", 
		(MP4Property**)&m_pTimeScaleProperty);
	if (success) {
		// default chunking is 1 second of samples
		m_durationPerChunk = m_pTimeScaleProperty->GetValue();
	}

	success &= m_pTrakAtom->FindProperty(
		"trak.tkhd.duration", 
		(MP4Property**)&m_pTrackDurationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.mdhd.duration", 
		(MP4Property**)&m_pMediaDurationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.tkhd.modificationTime", 
		(MP4Property**)&m_pTrackModificationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.mdhd.modificationTime", 
		(MP4Property**)&m_pMediaModificationProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.hdlr.handlerType",
		(MP4Property**)&m_pTypeProperty);

	// get handles on sample size information

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.sampleSize",
		(MP4Property**)&m_pStszFixedSampleSizeProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.sampleCount",
		(MP4Property**)&m_pStszSampleCountProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.entries.sampleSize",
		(MP4Property**)&m_pStszSampleSizeProperty);

	// get handles on information needed to map sample id's to file offsets

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entryCount",
		(MP4Property**)&m_pStscCountProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.firstChunk",
		(MP4Property**)&m_pStscFirstChunkProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.samplesPerChunk",
		(MP4Property**)&m_pStscSamplesPerChunkProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.sampleDescriptionIndex",
		(MP4Property**)&m_pStscSampleDescrIndexProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries.firstSample",
		(MP4Property**)&m_pStscFirstSampleProperty);

	bool haveStco = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stco.entryCount",
		(MP4Property**)&m_pChunkCountProperty);

	if (haveStco) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.stco.entries.chunkOffset",
			(MP4Property**)&m_pChunkOffsetProperty);
	} else {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.co64.entryCount",
			(MP4Property**)&m_pChunkCountProperty);

		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.co64.entries.chunkOffset",
			(MP4Property**)&m_pChunkOffsetProperty);
	}

	// get handles on sample timing info

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entryCount",
		(MP4Property**)&m_pSttsCountProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entries.sampleCount",
		(MP4Property**)&m_pSttsSampleCountProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entries.sampleDelta",
		(MP4Property**)&m_pSttsSampleDeltaProperty);
	
	// get handles on rendering offset info if it exists

	m_pCttsCountProperty = NULL;
	m_pCttsSampleCountProperty = NULL;
	m_pCttsSampleOffsetProperty = NULL;

	bool haveCtts = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.ctts.entryCount",
		(MP4Property**)&m_pCttsCountProperty);

	if (haveCtts) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.ctts.entries.sampleCount",
			(MP4Property**)&m_pCttsSampleCountProperty);

		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.ctts.entries.sampleOffset",
			(MP4Property**)&m_pCttsSampleOffsetProperty);
	}

	// get handles on sync sample info if it exists

	m_pStssCountProperty = NULL;
	m_pStssSampleProperty = NULL;

	bool haveStss = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stss.entryCount",
		(MP4Property**)&m_pStssCountProperty);

	if (haveStss) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.stss.entries.sampleNumber",
			(MP4Property**)&m_pStssSampleProperty);
	}

	// was everything found?
	if (!success) {
		throw new MP4Error("invalid track", "MP4Track::MP4Track");
	}
}

const char* MP4Track::GetType()
{
	return m_pTypeProperty->GetValue();
}

void MP4Track::SetType(const char* type) 
{
	m_pTypeProperty->SetValue(NormalizeTrackType(type));
}

void MP4Track::ReadSample(MP4SampleId sampleId,
	u_int8_t** ppBytes, u_int32_t* pNumBytes, 
	MP4Timestamp* pStartTime, MP4Duration* pDuration,
	MP4Duration* pRenderingOffset, bool* pIsSyncSample)
{
	if (sampleId == 0) {
		throw new MP4Error("sample id can't be zero", 
			"MP4Track::ReadSample");
	}

	u_int64_t fileOffset = GetSampleFileOffset(sampleId);

	u_int32_t sampleSize = GetSampleSize(sampleId);
	if (*pNumBytes != 0 && *pNumBytes < sampleSize) {
		throw new MP4Error("sample buffer is too small",
			 "MP4Track::ReadSample");
	}
	*pNumBytes = sampleSize;

	VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
		printf("ReadSample: id %u offset 0x"LLX" size %u (0x%x)\n",
			sampleId, fileOffset, *pNumBytes, *pNumBytes));

	// TBD could check would be to see if sample is in an mdat atom

	bool bufferMalloc = false;
	if (*ppBytes == NULL) {
		*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);
		bufferMalloc = true;
	}

	try { 
		m_pFile->SetPosition(fileOffset);
		m_pFile->ReadBytes(*ppBytes, *pNumBytes);

		if (pStartTime || pDuration) {
			GetSampleTimes(sampleId, pStartTime, pDuration);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  start "LLU" duration "LLD"\n",
					(pStartTime ? *pStartTime : 0), 
					(pDuration ? *pDuration : 0)));
		}
		if (pRenderingOffset) {
			*pRenderingOffset = GetSampleRenderingOffset(sampleId);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  renderingOffset "LLD"\n",
					*pRenderingOffset));
		}
		if (pIsSyncSample) {
			*pIsSyncSample = IsSyncSample(sampleId);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  isSyncSample %u\n",
					*pIsSyncSample));
		}
	}

	catch (MP4Error* e) {
		if (bufferMalloc) {
			// let's not leak memory
			MP4Free(*ppBytes);
			*ppBytes = NULL;
		}
		throw e;
	}
}

void MP4Track::WriteSample(u_int8_t* pBytes, u_int32_t numBytes,
	MP4Duration duration, MP4Duration renderingOffset, 
	bool isSyncSample)
{
	VERBOSE_WRITE_SAMPLE(m_pFile->GetVerbosity(),
		printf("WriteSample: id %u size %u (0x%x)\n",
			m_writeSampleId, numBytes, numBytes));

	if (pBytes == NULL) {
		throw new MP4Error("no sample data", "MP4WriteSample");
	}

	if (numBytes == 0) {
		throw new MP4Error("sample size is zero", "MP4WriteSample");
	}

	if (duration == MP4_INVALID_DURATION) {
		duration = GetFixedSampleDuration();
	}

	// append sample bytes to chunk buffer
	m_pChunkBuffer = (u_int8_t*)MP4Realloc(m_pChunkBuffer, 
		m_chunkBufferSize + numBytes);
	memcpy(&m_pChunkBuffer[m_chunkBufferSize], pBytes, numBytes);
	m_chunkBufferSize += numBytes;
	m_chunkSamples++;
	m_chunkDuration += duration;

	UpdateSampleSizes(m_writeSampleId, numBytes);

	UpdateSampleTimes(duration);

	UpdateRenderingOffsets(m_writeSampleId, renderingOffset);

	UpdateSyncSamples(m_writeSampleId, isSyncSample);

	if (IsChunkFull(m_writeSampleId)) {
		WriteChunk();
	}

	UpdateDurations(duration);

	UpdateModificationTimes();

	m_writeSampleId++;
}

void MP4Track::WriteChunk()
{
	if (m_chunkBufferSize == 0) {
		return;
	}

	u_int64_t chunkOffset = m_pFile->GetPosition();

	// write chunk buffer
	m_pFile->WriteBytes(m_pChunkBuffer, m_chunkBufferSize);

	VERBOSE_WRITE_SAMPLE(m_pFile->GetVerbosity(),
		printf("WriteChunk: offset 0x"LLX" size %u (0x%x) numSamples %u\n",
			chunkOffset, m_chunkBufferSize, m_chunkBufferSize, m_chunkSamples));

	UpdateSampleToChunk(m_writeSampleId, 
		m_pChunkCountProperty->GetValue() + 1, 
		m_chunkSamples);

	UpdateChunkOffsets(chunkOffset);

	// clean up chunk buffer
	MP4Free(m_pChunkBuffer);
	m_pChunkBuffer = NULL;
	m_chunkBufferSize = 0;
	m_chunkSamples = 0;
	m_chunkDuration = 0;
}

void MP4Track::FinishWrite()
{
	// write out any remaining samples in chunk buffer
	WriteChunk();
}

bool MP4Track::IsChunkFull(MP4SampleId sampleId)
{
	if (m_samplesPerChunk) {
		return m_chunkSamples >= m_samplesPerChunk;
	}

	ASSERT(m_durationPerChunk);
	return m_chunkDuration >= m_durationPerChunk;
}

u_int32_t MP4Track::GetNumberOfSamples()
{
	// if samples are variable size, we have the answer at hand in stsz
	if (m_pStszFixedSampleSizeProperty->GetValue() == 0) {
		return m_pStszSampleSizeProperty->GetCount();
	}

	// else samples are fixed size, 
	// we need to count up samples in stts
	u_int32_t numSamples = 0;
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	for (u_int32_t sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		numSamples += m_pSttsSampleCountProperty->GetValue(sttsIndex);
	}
	return numSamples;
}

u_int32_t MP4Track::GetSampleSize(MP4SampleId sampleId)
{
	u_int32_t fixedSampleSize = 
		m_pStszFixedSampleSizeProperty->GetValue(); 

	if (fixedSampleSize != 0) {
		return fixedSampleSize;
	}
	return m_pStszSampleSizeProperty->GetValue(sampleId - 1);
}

u_int32_t MP4Track::GetMaxSampleSize()
{
	u_int32_t fixedSampleSize = 
		m_pStszFixedSampleSizeProperty->GetValue(); 

	if (fixedSampleSize != 0) {
		return fixedSampleSize;
	}

	u_int32_t maxSampleSize = 0;
	u_int32_t numSamples = m_pStszSampleSizeProperty->GetCount();
	for (MP4SampleId sid = 1; sid <= numSamples; sid++) {
		u_int32_t sampleSize =
			m_pStszSampleSizeProperty->GetValue(sid - 1);
		if (sampleSize > maxSampleSize) {
			maxSampleSize = sampleSize;
		}
	}
	return maxSampleSize;
}

void MP4Track::UpdateSampleSizes(MP4SampleId sampleId, u_int32_t numBytes)
{
	// for first sample
	if (sampleId == 1) {
		// presume sample size is fixed
		m_pStszFixedSampleSizeProperty->SetValue(numBytes); 

	} else { // sampleId > 1
		u_int32_t fixedSampleSize = 
			m_pStszFixedSampleSizeProperty->GetValue(); 

		if (numBytes != fixedSampleSize) {
			// sample size is not fixed

			if (fixedSampleSize) {
				// need to clear fixed sample size
				m_pStszFixedSampleSizeProperty->SetValue(0); 

				// and create sizes for all previous samples
				for (MP4SampleId sid = 1; sid < sampleId; sid++) {
					m_pStszSampleSizeProperty->AddValue(fixedSampleSize);
					m_pStszSampleCountProperty->IncrementValue();
				}
			}

			// add size value for this sample
			m_pStszSampleSizeProperty->AddValue(numBytes);
			m_pStszSampleCountProperty->IncrementValue();
		}
	}
}

u_int64_t MP4Track::GetSampleFileOffset(MP4SampleId sampleId)
{
	u_int32_t stscIndex;
	u_int32_t numStscs = m_pStscCountProperty->GetValue();

	for (stscIndex = 0; stscIndex < numStscs; stscIndex++) {
		if (sampleId < m_pStscFirstSampleProperty->GetValue(stscIndex)) {
			ASSERT(stscIndex != 0);
			stscIndex -= 1;
			break;
		}
	}
	if (stscIndex == numStscs) {
		ASSERT(stscIndex != 0);
		stscIndex -= 1;
	}

	u_int32_t firstChunk = 
		m_pStscFirstChunkProperty->GetValue(stscIndex);

	MP4SampleId firstSample = 
		m_pStscFirstSampleProperty->GetValue(stscIndex);

	u_int32_t samplesPerChunk = 
		m_pStscSamplesPerChunkProperty->GetValue(stscIndex);

	u_int32_t chunkId = firstChunk +
		((sampleId - firstSample) / samplesPerChunk);

	u_int64_t chunkOffset = m_pChunkOffsetProperty->GetValue(chunkId - 1);

	MP4SampleId firstSampleInChunk = 
		sampleId - ((sampleId - firstSample) % samplesPerChunk);

	// need cumulative samples sizes from firstSample to sampleId - 1
	u_int32_t sampleOffset = 0;
	for (MP4SampleId i = firstSampleInChunk; i < sampleId; i++) {
		sampleOffset += GetSampleSize(i);
	}

	return chunkOffset + sampleOffset;
}

void MP4Track::UpdateSampleToChunk(MP4SampleId sampleId,
	 u_int32_t chunkId, u_int32_t samplesPerChunk)
{
	u_int32_t numStsc = m_pStscCountProperty->GetValue();

	// if samplesPerChunk == samplesPerChunk of last entry
	if (numStsc && samplesPerChunk == 
	  m_pStscSamplesPerChunkProperty->GetValue(numStsc-1)) {

		// nothing to do

	} else {
		// add stsc entry
		m_pStscFirstChunkProperty->AddValue(chunkId);
		m_pStscSamplesPerChunkProperty->AddValue(samplesPerChunk);
		m_pStscSampleDescrIndexProperty->AddValue(1);
		m_pStscFirstSampleProperty->AddValue(sampleId - samplesPerChunk + 1);

		m_pStscCountProperty->IncrementValue();
	}
}

void MP4Track::UpdateChunkOffsets(u_int64_t chunkOffset)
{
	if (m_pChunkOffsetProperty->GetType() == Integer32Property) {
		((MP4Integer32Property*)m_pChunkOffsetProperty)->AddValue(chunkOffset);
	} else { /* Integer64Property */
		((MP4Integer64Property*)m_pChunkOffsetProperty)->AddValue(chunkOffset);
	}
	m_pChunkCountProperty->IncrementValue();
}

MP4Duration MP4Track::GetFixedSampleDuration()
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	if (numStts == 0) {
		return m_fixedSampleDuration;
	}
	if (numStts != 1) {
		return MP4_INVALID_DURATION;	// sample duration is not fixed
	}
	return m_pSttsSampleDeltaProperty->GetValue(0);
}

bool MP4Track::SetFixedSampleDuration(MP4Duration duration)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	// setting this is only allowed before samples have been written
	if (numStts != 0) {
		return false;
	}
	m_fixedSampleDuration = duration;
	return true;
}

void MP4Track::GetSampleTimes(MP4SampleId sampleId,
	MP4Timestamp* pStartTime, MP4Duration* pDuration)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();
	MP4SampleId sid = 1;
	MP4Duration elapsed = 0;

	for (u_int32_t sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		u_int32_t sampleCount = 
			m_pSttsSampleCountProperty->GetValue(sttsIndex);
		u_int32_t sampleDelta = 
			m_pSttsSampleDeltaProperty->GetValue(sttsIndex);

		if (sampleId <= sid + sampleCount - 1) {
			if (pStartTime) {
				*pStartTime = elapsed + ((sampleId - sid) * sampleDelta);
			}
			if (pDuration) {
				*pDuration = sampleDelta;
			}
			return;
		}
		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;
	}

	throw new MP4Error("sample id out of range", 
		"MP4Track::GetSampleTimes");
}

MP4SampleId MP4Track::GetSampleIdFromTime(
	MP4Timestamp when, bool wantSyncSample)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();
	MP4SampleId sid = 1;
	MP4Duration elapsed = 0;

	for (u_int32_t sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		u_int32_t sampleCount = 
			m_pSttsSampleCountProperty->GetValue(sttsIndex);
		u_int32_t sampleDelta = 
			m_pSttsSampleDeltaProperty->GetValue(sttsIndex);

		if (sampleDelta == 0 && sttsIndex < numStts - 1) {
			VERBOSE_READ(m_pFile->GetVerbosity(),
				printf("Warning: Zero sample duration, stts entry %u\n",
				sttsIndex));
		}

		MP4Duration d = when - elapsed;

		if (d <= sampleCount * sampleDelta) {
			MP4SampleId sampleId = sid;
			if (sampleDelta) {
				sampleId += (d / sampleDelta);
				if (d % sampleDelta) {
					sampleId++;
				}
			}

			if (wantSyncSample) {
				return GetNextSyncSample(sampleId);
			}
			return sampleId;
		}

		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;
	}

	throw new MP4Error("time out of range", 
		"MP4Track::GetSampleIdFromTime");

	return 0; // satisfy MS compiler
}

void MP4Track::UpdateSampleTimes(MP4Duration duration)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	// if duration == duration of last entry
	if (numStts 
	  && duration == m_pSttsSampleDeltaProperty->GetValue(numStts-1)) {
		// increment last entry sampleCount
		m_pSttsSampleCountProperty->IncrementValue(numStts-1);

	} else {
		// add stts entry, sampleCount = 1, sampleDuration = duration
		m_pSttsSampleCountProperty->AddValue(1);
		m_pSttsSampleDeltaProperty->AddValue(duration);
		m_pSttsCountProperty->IncrementValue();;
	}
}

u_int32_t MP4Track::GetSampleRenderingOffset(MP4SampleId sampleId)
{
	if (m_pCttsCountProperty == NULL) {
		return 0;
	}

	u_int32_t numCtts = m_pCttsCountProperty->GetValue();

	if (numCtts == 0) {
		return 0;
	}

	MP4SampleId sid = 1;
	
	for (u_int32_t cttsIndex = 0; cttsIndex < numCtts; cttsIndex++) {
		u_int32_t sampleCount = 
			m_pCttsSampleCountProperty->GetValue(cttsIndex);

		if (sampleId <= sid + sampleCount - 1) {
			return m_pCttsSampleOffsetProperty->GetValue(cttsIndex);
		}
		sid += sampleCount;
	}

	throw new MP4Error("sample id out of range", 
		"MP4Track::GetSampleRenderingOffset");
	return 0; // satisfy MS compiler
}

void MP4Track::UpdateRenderingOffsets(MP4SampleId sampleId, 
	MP4Duration renderingOffset)
{
	// if ctts atom doesn't exist
	if (m_pCttsCountProperty == NULL) {

		// no rendering offset, so nothing to do
		if (renderingOffset == 0) {
			return;
		}

		// else create a ctts atom
		MP4Atom* pCttsAtom = AddAtom("trak.mdia.minf.stbl", "ctts");

		// and get handles on the properties
		pCttsAtom->FindProperty(
			"ctts.entryCount",
			(MP4Property**)&m_pCttsCountProperty);

		pCttsAtom->FindProperty(
			"ctts.entries.sampleCount",
			(MP4Property**)&m_pCttsSampleCountProperty);

		pCttsAtom->FindProperty(
			"ctts.entries.sampleOffset",
			(MP4Property**)&m_pCttsSampleOffsetProperty);

		// if this is not the first sample
		if (sampleId > 1) {
			// add a ctts entry for all previous samples
			// with rendering offset equal to zero
			m_pCttsSampleCountProperty->AddValue(sampleId - 1);
			m_pCttsSampleOffsetProperty->AddValue(0);
			m_pCttsCountProperty->IncrementValue();;
		}
	}

	// ctts atom exists (now)

	u_int32_t numCtts = m_pCttsCountProperty->GetValue();

	// if renderingOffset == renderingOffset of last entry
	if (numCtts && renderingOffset
	   == m_pCttsSampleOffsetProperty->GetValue(numCtts-1)) {

		// increment last entry sampleCount
		m_pCttsSampleCountProperty->IncrementValue(numCtts-1);

	} else {
		// add ctts entry, sampleCount = 1, sampleOffset = renderingOffset
		m_pCttsSampleCountProperty->AddValue(1);
		m_pCttsSampleOffsetProperty->AddValue(renderingOffset);
		m_pCttsCountProperty->IncrementValue();;
	}
}

bool MP4Track::IsSyncSample(MP4SampleId sampleId)
{
	if (m_pStssCountProperty == NULL) {
		return true;
	}

	u_int32_t numStss = m_pStssCountProperty->GetValue();
	
	for (u_int32_t stssIndex = 0; stssIndex < numStss; stssIndex++) {
		MP4SampleId syncSampleId = 
			m_pStssSampleProperty->GetValue(stssIndex);

		if (sampleId == syncSampleId) {
			return true;
		} 
		if (sampleId > syncSampleId) {
			break;
		}
	}

	return false;
}

// N.B. "next" is inclusive of this sample id
MP4SampleId MP4Track::GetNextSyncSample(MP4SampleId sampleId)
{
	if (m_pStssCountProperty == NULL) {
		return sampleId;
	}

	u_int32_t numStss = m_pStssCountProperty->GetValue();
	
	for (u_int32_t stssIndex = 0; stssIndex < numStss; stssIndex++) {
		MP4SampleId syncSampleId = 
			m_pStssSampleProperty->GetValue(stssIndex);

		if (sampleId > syncSampleId) {
			continue;
		}
		return syncSampleId;
	}

	// LATER check stsh for alternate sample

	return MP4_INVALID_SAMPLE_ID;
}

void MP4Track::UpdateSyncSamples(MP4SampleId sampleId, bool isSyncSample)
{
	if (isSyncSample) {
		// if stss atom exists, add entry
		if (m_pStssCountProperty) {
			m_pStssSampleProperty->AddValue(sampleId);
			m_pStssCountProperty->IncrementValue();
		} // else nothing to do (yet)

	} else { // !isSyncSample
		// if stss atom doesn't exist, create one
		if (m_pStssCountProperty == NULL) {

			MP4Atom* pStssAtom = AddAtom("trak.mdia.minf.stbl", "stss");

			pStssAtom->FindProperty(
				"stss.entryCount",
				(MP4Property**)&m_pStssCountProperty);

			pStssAtom->FindProperty(
				"stss.entries.sampleNumber",
				(MP4Property**)&m_pStssSampleProperty);

			// set values for all samples that came before this one
			for (MP4SampleId sid = 1; sid < sampleId; sid++) {
				m_pStssSampleProperty->AddValue(sid);
				m_pStssCountProperty->IncrementValue();
			}
		} // else nothing to do
	}
}

MP4Atom* MP4Track::AddAtom(char* parentName, char* childName)
{
	MP4Atom* pChildAtom = MP4Atom::CreateAtom(childName);

	MP4Atom* pParentAtom = m_pTrakAtom->FindAtom(parentName);
	ASSERT(pParentAtom);

	pParentAtom->AddChildAtom(pChildAtom);

	pChildAtom->Generate();

	return pChildAtom;
}

u_int32_t MP4Track::GetTimeScale()
{
	return m_pTimeScaleProperty->GetValue();
}

void MP4Track::UpdateDurations(MP4Duration duration)
{
	// update media, track, and movie durations
	m_pMediaDurationProperty->SetValue(
		m_pMediaDurationProperty->GetValue() + duration);

	MP4Duration movieDuration = ToMovieDuration(duration);
	m_pTrackDurationProperty->SetValue(
		m_pTrackDurationProperty->GetValue() + movieDuration);

	m_pFile->UpdateDuration(m_pTrackDurationProperty->GetValue());
}

MP4Duration MP4Track::ToMovieDuration(MP4Duration trackDuration)
{
	return (trackDuration * m_pFile->GetTimeScale()) 
		/ m_pTimeScaleProperty->GetValue();
}

void MP4Track::UpdateModificationTimes()
{
	// update media and track modification times
	MP4Timestamp now = MP4GetAbsTimestamp();
	m_pMediaModificationProperty->SetValue(now);
	m_pTrackModificationProperty->SetValue(now);
}

// map track type name aliases to official names

const char* MP4Track::NormalizeTrackType(const char* type)
{
	if (!strcasecmp(type, "vide")
	  || !strcasecmp(type, "video")
	  || !strcasecmp(type, "mp4v")) {
		return MP4_VIDEO_TRACK_TYPE;
	}

	if (!strcasecmp(type, "soun")
	  || !strcasecmp(type, "sound")
	  || !strcasecmp(type, "audio")
	  || !strcasecmp(type, "mp4a")) {
		return MP4_AUDIO_TRACK_TYPE;
	}

	if (!strcasecmp(type, "sdsm")
	  || !strcasecmp(type, "scene")
	  || !strcasecmp(type, "bifs")) {
		return MP4_SCENE_TRACK_TYPE;
	}

	if (!strcasecmp(type, "odsm")
	  || !strcasecmp(type, "od")) {
		return MP4_OD_TRACK_TYPE;
	}

	return type;
}

