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

#ifndef __MP4_TRACK_INCLUDED__
#define __MP4_TRACK_INCLUDED__

// forward declarations
class MP4File;
class MP4Atom;
class MP4Property;
class MP4IntegerProperty;
class MP4Integer32Property;
class MP4Integer64Property;
class MP4StringProperty;

class MP4Track {
public:
	MP4Track(MP4File* pFile, MP4Atom* pTrakAtom);

	MP4TrackId GetId() {
		return m_trackId;
	}

	const char* GetType();

	void SetType(const char* type);

	MP4Atom* GetTrakAtom() {
		return m_pTrakAtom;
	}

	void ReadSample(
		// input parameters
		MP4SampleId sampleId,
		// output parameters
		u_int8_t** ppBytes, 
		u_int32_t* pNumBytes, 
		MP4Timestamp* pStartTime = NULL, 
		MP4Duration* pDuration = NULL,
		MP4Duration* pRenderingOffset = NULL, 
		bool* pIsSyncSample = NULL);

	void WriteSample(
		u_int8_t* pBytes, 
		u_int32_t numBytes,
		MP4Duration duration = 0,
		MP4Duration renderingOffset = 0, 
		bool isSyncSample = true);

	void FinishWrite();

	u_int32_t	GetTimeScale();
	u_int32_t	GetNumberOfSamples();
	u_int32_t	GetSampleSize(MP4SampleId sampleId);
	u_int32_t	GetMaxSampleSize();

	MP4Duration GetFixedSampleDuration();
	bool SetFixedSampleDuration(MP4Duration duration);

	MP4SampleId GetSampleIdFromTime(MP4Timestamp when, 
		bool wantSyncSample = false);

	static const char* NormalizeTrackType(const char* type);

protected:
	u_int64_t	GetSampleFileOffset(MP4SampleId sampleId);
	void		GetSampleTimes(MP4SampleId sampleId,
					MP4Timestamp* pStartTime, MP4Duration* pDuration);
	u_int32_t	GetSampleRenderingOffset(MP4SampleId sampleId);
	bool		IsSyncSample(MP4SampleId sampleId);
	MP4SampleId	GetNextSyncSample(MP4SampleId sampleId);

	void UpdateSampleSizes(MP4SampleId sampleId, 
		u_int32_t numBytes);
	bool IsChunkFull(MP4SampleId sampleId);
	void UpdateSampleToChunk(MP4SampleId sampleId,
		 u_int32_t chunkId, u_int32_t samplesPerChunk);
	void UpdateChunkOffsets(u_int64_t chunkOffset);
	void UpdateSampleTimes(MP4Duration duration);
	void UpdateRenderingOffsets(MP4SampleId sampleId, 
		MP4Duration renderingOffset);
	void UpdateSyncSamples(MP4SampleId sampleId, 
		bool isSyncSample);

	MP4Atom* AddAtom(char* parentName, char* childName);

	void UpdateDurations(MP4Duration duration);
	MP4Duration ToMovieDuration(MP4Duration trackDuration);

	void UpdateModificationTimes();

	void WriteChunk();

protected:
	MP4File*	m_pFile;
	MP4Atom* 	m_pTrakAtom;		// moov.trak[]
	MP4TrackId	m_trackId;			// moov.trak[].tkhd.trackId
	MP4StringProperty* m_pTypeProperty;	// moov.trak[].mdia.hdlr.handlerType

	// for writing
	MP4SampleId m_writeSampleId;
	MP4Duration m_fixedSampleDuration;
	u_int8_t* 	m_pChunkBuffer;
	u_int32_t	m_chunkBufferSize;
	u_int32_t	m_chunkSamples;
	MP4Duration m_chunkDuration;

	// controls for chunking
	u_int32_t 	m_samplesPerChunk;
	MP4Duration m_durationPerChunk;

	MP4Integer32Property* m_pTimeScaleProperty;
	MP4IntegerProperty* m_pTrackDurationProperty;		// 32 or 64 bits
	MP4IntegerProperty* m_pMediaDurationProperty;		// 32 or 64 bits
	MP4IntegerProperty* m_pTrackModificationProperty;	// 32 or 64 bits
	MP4IntegerProperty* m_pMediaModificationProperty;	// 32 or 64 bits

	MP4Integer32Property* m_pStszFixedSampleSizeProperty;
	MP4Integer32Property* m_pStszSampleCountProperty;
	MP4Integer32Property* m_pStszSampleSizeProperty;

	MP4Integer32Property* m_pStscCountProperty;
	MP4Integer32Property* m_pStscFirstChunkProperty;
	MP4Integer32Property* m_pStscSamplesPerChunkProperty;
	MP4Integer32Property* m_pStscSampleDescrIndexProperty;
	MP4Integer32Property* m_pStscFirstSampleProperty;

	MP4Integer32Property* m_pChunkCountProperty;
	MP4IntegerProperty* m_pChunkOffsetProperty;			// 32 or 64 bits

	MP4Integer32Property* m_pSttsCountProperty;
	MP4Integer32Property* m_pSttsSampleCountProperty;
	MP4Integer32Property* m_pSttsSampleDeltaProperty;

	MP4Integer32Property* m_pCttsCountProperty;
	MP4Integer32Property* m_pCttsSampleCountProperty;
	MP4Integer32Property* m_pCttsSampleOffsetProperty;

	MP4Integer32Property* m_pStssCountProperty;
	MP4Integer32Property* m_pStssSampleProperty;
};

MP4ARRAY_DECL(MP4Track, MP4Track*);

#endif /* __MP4_TRACK_INCLUDED__ */
