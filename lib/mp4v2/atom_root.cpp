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

MP4RootAtom::MP4RootAtom() 
	: MP4Atom(NULL)
{
	ExpectChildAtom("ftyp", Required, OnlyOne);
	ExpectChildAtom("moov", Required, OnlyOne);
	ExpectChildAtom("mdat", Optional, Many);
	ExpectChildAtom("free", Optional, Many);
	ExpectChildAtom("skip", Optional, Many);
	ExpectChildAtom("udta", Optional, Many);
	ExpectChildAtom("moof", Optional, Many);
}

void MP4RootAtom::BeginWrite(bool use64) 
{
	u_int32_t size = m_pChildAtoms.Size();
	u_int32_t i;

	// write out ftyp atom
	for (i = 0; i < size; i++) {
		u_int32_t atomType = ATOMID(m_pChildAtoms[i]->GetType());
		if (atomType == ATOMID("ftyp")) {
			m_pChildAtoms[i]->Write();
		}
	}

	// begin writing mdat atom
	for (i = 0; i < size; i++) {
		u_int32_t atomType = ATOMID(m_pChildAtoms[i]->GetType());
		if (atomType == ATOMID("mdat")) {
			m_pChildAtoms[i]->BeginWrite(m_pFile->Use64Bits());
			break;
		}
	}
}

void MP4RootAtom::Write()
{
	u_int32_t size = m_pChildAtoms.Size();
	u_int32_t i;
	// finish writing mdat atom
	for (i = 0; i < size; i++) {
		u_int32_t atomType = ATOMID(m_pChildAtoms[i]->GetType());
		if (atomType == ATOMID("mdat")) {
			m_pChildAtoms[i]->FinishWrite(m_pFile->Use64Bits());
			break;
		}
	}

	// write all other atoms other than ftyp and mdat
	for (i = 0; i < size; i++) {
		u_int32_t atomType = ATOMID(m_pChildAtoms[i]->GetType());
		if (atomType != ATOMID("ftyp") && atomType != ATOMID("mdat")) {
			m_pChildAtoms[i]->Write();
		}
	}
}

void MP4RootAtom::FinishWrite(bool use64)
{
	// no-op
}
