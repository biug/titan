//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef SKT_H
#define SKT_H

#include <BWAPI.h>
#include "strat.h"
#include "../defs.h"
#include "../utils.h"


namespace iron
{

	class MyUnit;

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class Locutus
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////
	//

	class SKT : public Strat
	{
	public:
		SKT();
		~SKT();

		string							Name() const override { return "SKT"; }
		string							StateDescription() const override;

		bool							Active() const { return m_active; }

	private:
		void							OnFrame_v() override;

		bool							m_active = false;
	};


} // namespace iron


#endif

