//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef HANGINGBASE_H
#define HANGINGBASE_H

#include <BWAPI.h>
#include "behavior.h"
#include "../utils.h"


namespace iron
{

	class MyUnit;
		FORWARD_DECLARE_MY(Terran_SCV)
		FORWARD_DECLARE_MY(Terran_Siege_Tank_Tank_Mode)
		FORWARD_DECLARE_MY(Terran_Dropship)

		//////////////////////////////////////////////////////////////////////////////////////////////
		//                                                                                          //
		//                                  class HangingBase
		//                                                                                          //
		//////////////////////////////////////////////////////////////////////////////////////////////
		//

	class HangingBase : public Behavior<MyUnit>
	{
	public:
		static const vector<HangingBase *> &	Instances() { return m_Instances; }

		enum state_t { entering, flying, landing };

		HangingBase(My<Terran_Dropship> * pAgent);
		~HangingBase();

		const HangingBase *			IsHangingBase() const override	{ return this; }
		HangingBase *				IsHangingBase() override		{ return this; }

		string						Name() const override { return "hangingBase"; }
		string						StateName() const override;

		BWAPI::Color				GetColor() const override { return Colors::White; }
		Text::Enum					GetTextColor() const override { return Text::White; }

		void						OnFrame_v() override;

		bool						CanRepair(const MyBWAPIUnit *, int) const override { return false; }
		bool						CanChase(const HisUnit *) const override { return false; }

		state_t						State() const { CI(this); return m_state; }

		My<Terran_Dropship> *		ThisDropship() const;

		vector<My<Terran_Vulture> *>		GetVultures() const;

	private:
		void						ChangeState(state_t st);
		int							Targets(MyUnit * u);
		bool						NearEnemy(MyUnit * u);
		void						OnFrame_entering();
		void						OnFrame_flying();
		void						OnFrame_landing();
		void						OnOtherBWAPIUnitDestroyed_v(BWAPIUnit * other) override;
		Position					NextEnemyBase();

		state_t						m_state = entering;
		frame_t						m_inLandingSince = 0;
		frame_t						m_lastMove = 0;
		Position					m_target;

		vector<MyUnit *>			m_Units;
		vector<Position>			m_airlines;
		int							m_airlineIndex = -1;
		int							m_targetIndex = -1;

		static vector<HangingBase *>	m_Instances;
	};



} // namespace iron


#endif

