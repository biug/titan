//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef PURPLESWARM_H
#define PURPLESWARM_H

#include <BWAPI.h>
#include "strat.h"
#include "../defs.h"
#include "../utils.h"


namespace iron
{

	class MyUnit;

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class purpleswarm
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////
	//

	class Purpleswarm : public Strat
	{
	public:
		Purpleswarm();
		~Purpleswarm();

		string							Name() const override { return "Purpleswarm"; }
		string							StateDescription() const override;

		bool							Detected() const { return m_detected; }

	private:
		void							OnFrame_v() override;

		bool							m_detected = false;
		int								m_firstDropFrame = 0;
		const Area *					m_pNaturalArea = nullptr;
	};


} // namespace iron


#endif

