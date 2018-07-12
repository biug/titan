//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef GUARDLOC_H
#define GUARDLOC_H

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

	class GuardLoc : public Behavior<MyUnit>
	{
	public:
		static const vector<GuardLoc *> &	Instances() { return m_Instances; }

		GuardLoc(MyUnit * pAgent, const Position target);
		~GuardLoc();

		enum state_t { moving, guarding };

		const GuardLoc *			IsGuardLoc() const override { return this; }
		GuardLoc *					IsGuardLoc() override { return this; }

		string						Name() const override { return "guardLoc"; }
		string						StateName() const override;


		BWAPI::Color				GetColor() const override { return Colors::Yellow; }
		Text::Enum					GetTextColor() const override { return Text::Yellow; }

		void						OnFrame_v() override;

		bool						CanRepair(const MyBWAPIUnit *, int) const override { return false; }
		bool						CanChase(const HisUnit *) const override { return false; }

		state_t						State() const { CI(this); return m_state; }
		Position					Target() const { CI(this); return m_target; }

	private:
		void						ChangeState(state_t st);
		void						OnFrame_moving();
		void						OnFrame_guarding();

		bool						BuildOnAgent();

		state_t						m_state = moving;
		frame_t						m_inMovingSince = 0;
		frame_t						m_inGuardingSince = 0;
		Position					m_target = Positions::None;

		static vector<GuardLoc *>	m_Instances;
	};



} // namespace iron


#endif

