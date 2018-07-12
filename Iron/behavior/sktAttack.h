//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef SKTATTACK_H
#define SKTATTACK_H

#include <BWAPI.h>
#include "behavior.h"
#include "../utils.h"


namespace iron
{

	class MyUnit;

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class GuardLoc
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////
	//

	class SKTAttack : public Behavior<MyUnit>
	{
	public:
		static const vector<SKTAttack *> &	Instances() { return m_Instances; }

		SKTAttack(MyUnit * pAgent);
		~SKTAttack();

		enum state_t { guard, outdoor, attack, raze };

		const SKTAttack *			IsSKTAttack() const override { return this; }
		SKTAttack *					IsSKTAttack() override { return this; }

		string						Name() const override { return "sktAttack"; }
		string						StateName() const override;


		BWAPI::Color				GetColor() const override { return Colors::Yellow; }
		Text::Enum					GetTextColor() const override { return Text::Yellow; }

		void						OnFrame_v() override;

		bool						CanRepair(const MyBWAPIUnit *, int) const override { return false; }
		bool						CanChase(const HisUnit *) const override { return false; }
		bool						HasTank() const { return m_firstTank; }
		static int					Count(UnitType type);

		state_t						State() const { CI(this); return m_state; }
		HisUnit *					Target() const { CI(this); return m_target; }

	private:
		void						ChangeState(state_t st);
		void						OnFrame_guard();
		void						OnFrame_outdoor();
		void						OnFrame_attack();
		void                        OnFrame_raze();
		void						ChooseAttackTarget();
		void						SmartAttack(Position p);

		state_t						m_state = guard;
		HisUnit *					m_target;

		static vector<SKTAttack *>	m_Instances;
		static Position				m_SKTGuardPos;
		static Position				m_SKTOutdoorPos;
		static bool					m_firstTank;
	};



} // namespace iron


#endif

