//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef LOCUTUS_H
#define LOCUTUS_H

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

	class Locutus : public Strat
	{
	public:
		Locutus();
		~Locutus();

		string							Name() const override { return "Locutus"; }
		string							StateDescription() const override;

		bool							Detected() const { return m_detected; }

	private:
		void							OnFrame_v() override;

		bool							m_detected = false;
		int								m_firstDropFrame = 0;
	};


} // namespace iron


#endif

