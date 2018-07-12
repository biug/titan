//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "tankAdvance.h"
#include "guardNatural.h"
#include "strategy.h"
#include "../units/army.h"
#include "../units/his.h"
#include "../units/cc.h"
#include "../units/comsat.h"
#include "../Iron.h"

namespace { auto & bw = Broodwar; }




namespace iron
{


	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class TankAdvance
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////


	TankAdvance::TankAdvance()
	{
	}


	TankAdvance::~TankAdvance()
	{
		ai()->GetStrategy()->SetMinScoutingSCVs(1);
	}


	string TankAdvance::StateDescription() const
	{
		if (!m_active) return "-";
		if (m_active) return "active";

		return "-";
	}

	void TankAdvance::OnFrame_v()
	{
		// 主基地被消灭，不激活
		if (him().StartingBaseDestroyed())
		{
			m_active = false;
			Discard();
		}
		// 敌人不是locutus，不激活
		if (!(him().Player()->getName() == "locutus" || him().Player()->getName() == "Locutus"))
		{
			m_active = false;
			Discard();
		}

		// 判断guardNatural是否被激活
		if (!m_guardNatural)
			if (ai()->GetStrategy()->Active<GuardNatural>())
				m_guardNatural = true;

		// 如果激活guardNatural
		if (m_guardNatural)
		{
			// 如果仍然激活GuardNatural，跳过
			if (ai()->GetStrategy()->Active<GuardNatural>())
			{
				m_active = false;
				return;
			}
			// 记录guard结束时间
			if (m_guardNaturalEndFrame == 0)
				m_guardNaturalEndFrame = ai()->Frame();
			// guard结束3秒后，激活
			if (ai()->Frame() - m_guardNaturalEndFrame > 3 * 24)
			{
				m_active = true;
				// 初始化上次更新前线的时间
				if (m_lastFrontlineFrame == 0)
					m_lastFrontlineFrame = ai()->Frame();
			}

			// 更新前线
			UpdateFrontline();
			// 更新落后的坦克
			UpdateBackwardTanks();
			// 雷达扫描
			CastScannerSweep();
		}
	}


	void TankAdvance::UpdateFrontline()
	{
		m_frontline = Positions::None;
		for (auto & u : me().Units())
		{
			if (!u->Unit() || !u->Unit()->exists() || !u->Completed()) continue;
			if (!(u->Is(Terran_Siege_Tank_Tank_Mode) || u->Is(Terran_Siege_Tank_Siege_Mode))) continue;
			// 距离最远的坦克
			if (m_frontline == Positions::None || groundDist(u->Pos(), him().StartingBase()->Center()) < groundDist(m_frontline, him().StartingBase()->Center()))
				m_frontline = u->Pos();
		}
	}

	void TankAdvance::UpdateBackwardTanks()
	{
		vector<pair<MyUnit*, int>> tanks;
		m_backwardTanks.clear();
		for (auto & u : me().Units())
		{
			if (!u->Is(Terran_Siege_Tank_Tank_Mode) && !u->Is(Terran_Siege_Tank_Siege_Mode)) continue;
			if (!u->Completed()) continue;
			if (!u->GuardInNatural()) continue;
			tanks.push_back(make_pair(u, groundDist(u->Pos(), m_frontline)));
		}
		// 按照距前线的远近给坦克排序
		sort(tanks.begin(), tanks.end(), [](const pair<MyUnit*, int>& p1, const pair<MyUnit*, int>&p2) {
			return p1.second > p2.second;
		});

		// 选出最远的1/3的坦克
		int tankCount = me().CompletedUnits(Terran_Siege_Tank_Tank_Mode) + me().CompletedUnits(Terran_Siege_Tank_Siege_Mode);
		for (auto & tank : tanks)
		{
			m_backwardTanks.insert(tank.first);
			if (m_backwardTanks.size() >= tankCount / 3) break;
			if (m_backwardTanks.size() >= 4) break;
		}
	}

	void TankAdvance::CastScannerSweep()
	{
		if (ai()->Frame() - m_lastScanFrame < 35 * 60) return;

		if (!(me().BestComsat() && me().BestComsat()->CanAcceptCommand())) return;

		//计算使用位置
		assert_throw(him().StartingBase());
		Position targetPosition = him().StartingBase()->Center();
		if (m_frontline != Positions::None)
		{
			int length;
			auto & path = ai()->GetMap().GetPath(m_frontline, him().StartingBase()->Center(), &length);
			if (path.size() > 0)
			{
				Position chokePos = (Position)path[0]->Center();
				int lineToChoke = m_frontline.getApproxDistance(chokePos);
				int range = bw->self()->sightRange(Spell_Scanner_Sweep) + bw->self()->sightRange(Terran_Siege_Tank_Siege_Mode);
				if (lineToChoke >= range)
					targetPosition = (m_frontline * (lineToChoke - range) + chokePos * range) / lineToChoke;
				else
					targetPosition = (chokePos * range - m_frontline * (range - lineToChoke)) / lineToChoke;
			}
		}
		//使用
		me().BestComsat()->Scan(targetPosition);
		m_lastScanFrame = ai()->Frame();
	}


} // namespace iron



