//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "hangingBase.h"
#include "../units/factory.h"
#include "../units/starport.h"
#include "../units/cc.h"
#include "../bwem/src/map.h"
#include "../Iron.h"
#include "../units/starport.h"
#include "../units/him.h"
#include "../units/his.h"
#include "guardLoc.h"
#include "../strategy/expand.h"

namespace { auto & bw = Broodwar; }


namespace iron
{

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class HangingBase
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////

	vector<HangingBase *> HangingBase::m_Instances;

	HangingBase::HangingBase(My<Terran_Dropship> * pAgent)
		: Behavior(pAgent, behavior_t::HangingBase)
	{
		assert_throw(Agent()->Is(Terran_Dropship));

		m_Units = ThisDropship()->LoadedUnits();

		PUSH_BACK_UNCONTAINED_ELEMENT(m_Instances, this);

		// 计算航线
		int minX = 0;
		int minY = 0;
		int maxX = bw->mapWidth() - 1;
		int maxY = bw->mapHeight() - 1;

		// Add vertices down the left edge.
		for (int y = minY; y <= maxY; y += 5)
		{
			m_airlines.push_back(BWAPI::Position(BWAPI::TilePosition(minX, y)) + BWAPI::Position(16, 16));
		}
		// Add vertices across the bottom.
		for (int x = minX; x <= maxX; x += 5)
		{
			m_airlines.push_back(BWAPI::Position(BWAPI::TilePosition(x, maxY)) + BWAPI::Position(16, 16));
		}
		// Add vertices up the right edge.
		for (int y = maxY; y >= minY; y -= 5)
		{
			m_airlines.push_back(BWAPI::Position(BWAPI::TilePosition(maxX, y)) + BWAPI::Position(16, 16));
		}
		// Add vertices across the top back to the origin.
		for (int x = maxX; x >= minX; x -= 5)
		{
			m_airlines.push_back(BWAPI::Position(BWAPI::TilePosition(x, minY)) + BWAPI::Position(16, 16));
		}

		m_target = me().StartingBase()->Center();

		///	ai()->SetDelay(100);
	}


	HangingBase::~HangingBase()
	{
#if !DEV
		try //3
#endif
		{
			assert_throw(contains(m_Instances, this));
			really_remove(m_Instances, this);
		}
#if !DEV
		catch (...) {} //3
#endif
	}


	void HangingBase::ChangeState(state_t st)
	{
		CI(this);
		assert_throw(m_state != st);

		m_state = st; OnStateChanged();
	}


	string HangingBase::StateName() const
	{
		CI(this);
		switch (State())
		{
		case entering:		return "entering";
		case flying:		return "flying";
		case landing:		return "landing";
		default:			return "?";
		}
	}

	void HangingBase::OnOtherBWAPIUnitDestroyed_v(BWAPIUnit * other)
	{
		CI(this);

		if (MyUnit * u = other->IsMyUnit())
			really_remove(m_Units, u);
	}

	int HangingBase::Targets(MyUnit * u)
	{
		int targets = 0;
		for (const auto & target : him().Units())
			if (!target->InFog() && target->Unit()->exists())
				if (roundedDist(target->Pos(), u->Pos()) < 6 * 32)
					if (target->Type() == Protoss_Dragoon)
						++targets;
		return targets;
	}

	bool HangingBase::NearEnemy(MyUnit * u)
	{
		for (const auto & target : him().Units())
			if (!target->InFog() && target->Unit()->exists())
				if (roundedDist(target->Pos(), u->Pos()) < 10 * 32)
				{
					return true;
				}
		return false;
	}

	Position HangingBase::NextEnemyBase()
	{
		auto pHisMain = him().StartingVBase();
		if (!pHisMain) return me().StartingBase()->Center();
		auto pHisNatural = findNatural(pHisMain);
		if (!pHisNatural) return me().StartingBase()->Center();

		// 遍历基地
		int findOne = 30;
		while (--findOne)
		{
			// 首先找已知的基地
			auto & hisBase = him().Buildings(him().Race().getCenter())[rand() % him().Buildings(him().Race().getCenter()).size()];
			if (hisBase->Pos().getApproxDistance(pHisMain->BWEMPart()->Center()) > 3 * 32)
				if (hisBase->Pos().getApproxDistance(pHisNatural->BWEMPart()->Center()) > 3 * 32)
					return hisBase->Pos();

			// 遍历未知基地
			auto & base = ai()->GetVMap().Bases()[rand() % ai()->GetVMap().Bases().size()];
			// 不是对方主基地
			if (&base != pHisMain)
			// 不是对方二矿
			if (&base != pHisNatural)
			// 不是当前所在地
			if (Agent()->Pos().getApproxDistance(base.BWEMPart()->Center()) > 12 * 32)
			// 还有水晶
			if (!base.BWEMPart()->Minerals().empty())
			{
				bool notMe = true;
				for (auto & center : me().Buildings(Terran_Command_Center))
					if (center->Pos().getApproxDistance(base.BWEMPart()->Center()) < 6 * 32)
						notMe = false;
				// 不是我的矿
				if (notMe)
				{
					int length;
					ai()->GetMap().GetPath(base.BWEMPart()->Center(), pHisMain->BWEMPart()->Center(), &length);
					// 可达
					if (length > 0)
					{
						// 返回这个位置
						return base.BWEMPart()->Center();
					}
				}
			}
		}
		return me().StartingBase()->Center();
	}

	My<Terran_Dropship> * HangingBase::ThisDropship() const
	{
		CI(this);
		return Agent()->IsMy<Terran_Dropship>();
	}

	vector<My<Terran_Vulture> *> HangingBase::GetVultures() const
	{
		vector<My<Terran_Vulture> *> List;

		for (BWAPIUnit * u : m_Units)
			if (My<Terran_Vulture> * pSCV = u->IsMyUnit()->IsMy<Terran_Vulture>())
				List.push_back(pSCV);

		return List;
	}

	void HangingBase::OnFrame_v()
	{
		CI(this);
		///	if (m_repairOnly) for (int i = 0 ; i < 3 ; ++i) bw->drawCircleMap(Agent()->Pos(), 64 + i, Colors::Yellow);

		if (!Agent()->CanAcceptCommand()) return;

		bw->drawLineMap(Agent()->Pos(), m_target, Colors::Red);
		if (State() == flying)
			if (m_target != me().StartingBase()->Center())
				for (const auto & target : me().Units())
					if (roundedDist(target->Pos(), m_target) < 10 * 32)
					{
						m_airlineIndex = m_targetIndex = -1;
						m_target = NextEnemyBase();
						break;
					}

		switch (State())
		{
		case entering:		OnFrame_entering(); break;
		case flying:		OnFrame_flying(); break;
		case landing:		OnFrame_landing(); break;
		default: assert_throw(false);
		}
	}

	void HangingBase::OnFrame_entering()
	{
		CI(this);
		// 加载单位
		if (Agent()->As<Terran_Dropship>()->LoadedUnits().size() < 4)
		{
			for (auto & u : me().Units())
				if (!u->Loaded())
					if (u->Is(Terran_Vulture))
						if (u->GuardInLoc())
							// 半径8内选择
							if (u->Pos().getApproxDistance(Agent()->Pos()) < 32 * 8)
							{
								// 如果距离小于8，装载
								if (u->Pos().getApproxDistance(Agent()->Pos()) < 8)
									return Agent()->As<Terran_Dropship>()->Load(u);
								// 否则飞过去
								else
									return Agent()->Move(u->Pos());
							}
		}

		// 没有单位可以加载
		if (Agent()->As<Terran_Dropship>()->LoadedUnits().size() < 2 || Targets(Agent()) > 0)
		{
			// 如果数量小于2，回家
			m_target = me().StartingBase()->Center();
			// 在家不动
			return Agent()->Move(m_target);
		}
		else
			// 否则，飞去下一个基地
			m_target = NextEnemyBase();
		// 将加载单位target设置为新的target
		for (auto & u : Agent()->As<Terran_Dropship>()->LoadedUnits())
			u->ChangeBehavior<GuardLoc>(u, m_target);
		// 重置航线
		m_airlineIndex = m_targetIndex = -1;
		// 进入flying
		bw->sendText("flying to (%d,%d)", m_target.x / 32, m_target.y / 32);
		ChangeState(flying);
	}

	void HangingBase::OnFrame_flying()
	{
		CI(this);
		// 贴边飞行
		if (m_airlineIndex < 0)
		{
			for (int i = 0; i < m_airlines.size(); ++i)
			{
				if (m_airlineIndex == -1 ||
					m_airlines[i].getApproxDistance(Agent()->Pos()) < m_airlines[m_airlineIndex].getApproxDistance(Agent()->Pos()))
					m_airlineIndex = i;
				if (m_targetIndex == -1 ||
					m_airlines[i].getApproxDistance(m_target) < m_airlines[m_targetIndex].getApproxDistance(m_target))
					m_targetIndex = i;
				bw->drawCircleMap(m_airlines[i], 16, Colors::Red);
			}
		}
		bw->drawCircleMap(m_airlines[m_airlineIndex], 16, Colors::Red, true);
		bw->drawCircleMap(m_airlines[m_targetIndex], 16, Colors::Red, true);
		// 到达目标
		if (m_airlineIndex == m_targetIndex)
		{
			m_inLandingSince = 0;
			return ChangeState(landing);
		}
		else
		{
			// 没到达下一航点，继续飞行
			if (Agent()->Pos().getApproxDistance(m_airlines[m_airlineIndex]) > 32 * 2)
				return Agent()->Move(m_airlines[m_airlineIndex]);
			// 计算下一航点
			if (abs(m_targetIndex - m_airlineIndex) <= m_airlines.size() / 2)
			{
				if (m_airlineIndex < m_targetIndex) m_airlineIndex += 1;
				else m_airlineIndex -= 1;
			}
			else
			{
				if (m_airlineIndex > m_targetIndex) m_airlineIndex += 1;
				else m_airlineIndex -= 1;
			}
			if (m_airlineIndex < 0) m_airlineIndex = m_airlines.size() - 1;
			if (m_airlineIndex == m_airlines.size()) m_airlineIndex = 0;
		}
	}

	void HangingBase::OnFrame_landing()
	{
		CI(this);

		if (m_inLandingSince == 0) m_inLandingSince = ai()->Frame();

		// 如果周围有龙骑，找下一个基地
		if (Targets(Agent()) > 2)
		{
			bw->sendText("dragoon around");
			return ChangeState(entering);
		}
		// 如果超过三十秒，找下一个基地
		if (ai()->Frame() - m_inLandingSince > 30 * 24)
		{
			bw->sendText("wait for too long");
			return ChangeState(entering);
		}
		// 如果没有地方单位，找下一个基地
		if (!NearEnemy(Agent()))
		{
			bw->sendText("no enemy around");
			return ChangeState(entering);
		}

		auto agentArea = ai()->GetMap().GetArea(WalkPosition(Agent()->Pos()));
		auto targetArea = ai()->GetMap().GetArea(WalkPosition(m_target));
		// 如果不连通，继续飞行
		if (agentArea != targetArea) return Agent()->Move(m_target);
		// 如果可以放下，放下
		if (Agent()->As<Terran_Dropship>()->CanUnload() && !Agent()->As<Terran_Dropship>()->LoadedUnits().empty())
		{
			return Agent()->As<Terran_Dropship>()->Unload(Agent()->As<Terran_Dropship>()->LoadedUnits()[0]);
		}
		//else
		//{
		//	int i1 = targetArea->TopLeft().x, j1 = targetArea->TopLeft().y, i2 = targetArea->BottomRight().x, j2 = targetArea->BottomRight().y;
		//	// 找放置点
		//	for (int j = j1; j <= j2; ++j)
		//		for (int i = i1; i <= i2; ++i)
		//		{
		//			Position target = Position(TilePosition(i, j)) + Position(16, 16);
		//			CHECK_POS(target);
		//			// 半径为6的地方找位置
		//			if (ai()->GetMap().GetArea(WalkPosition(target)) == targetArea)
		//				if (m_target.getApproxDistance(target) < 6 * 32)
		//			{
		//			}
		//		}
		//}
	}



} // namespace iron
