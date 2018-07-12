//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef TANKADVANCE_H
#define TANKADVANCE_H

#include <BWAPI.h>
#include "strat.h"
#include "../defs.h"
#include "../utils.h"


namespace iron
{

	class MyUnit;

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class TankAdvance
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////
	//

	class TankAdvance : public Strat
	{
	public:
		TankAdvance();
		~TankAdvance();

		string							Name() const override { return "TankAdvance"; }
		string							StateDescription() const override;

		bool							Active() const { return m_active; }
		Position						GetFrontLine() const { return m_frontline; }
		bool							FindBackwardTanks(MyUnit * u) const { return m_backwardTanks.find(u) != m_backwardTanks.end(); }

	private:
		void							OnFrame_v() override;
		void							UpdateFrontline();
		void							UpdateBackwardTanks();
		void							CastScannerSweep();

		bool							m_active = false;
		bool							m_guardNatural = false;
		int								m_guardNaturalEndFrame = 0;

		int								m_lastFrontlineFrame = 0;
		int								m_lastScanFrame = 0;

		Position						m_frontline = Positions::None;
		unordered_set<MyUnit*>			m_backwardTanks;
	};


} // namespace iron


#endif

