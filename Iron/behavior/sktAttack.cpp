//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "sktAttack.h"
#include "walking.h"
#include "repairing.h"
#include "sieging.h"
#include "kiting.h"
#include "razing.h"
#include "raiding.h"
#include "fleeing.h"
#include "constructing.h"
#include "fighting.h"
#include "defaultBehavior.h"
#include "../units/factory.h"
#include "../units/bunker.h"
#include "../strategy/strategy.h"
#include "../strategy/baseDefense.h"
#include "../strategy/locutus.h"
#include "../strategy/walling.h"
#include "../strategy/tankAdvance.h"
#include "../strategy/expand.h"
#include "../Iron.h"
#include "../territory/vmap.h"
#include "../territory/vgridMap.h"
#include "../units/cc.h"

namespace { auto & bw = Broodwar; }


namespace iron
{

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class GuardLoc
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////

	vector<SKTAttack *> SKTAttack::m_Instances;
	Position			SKTAttack::m_SKTGuardPos = Positions::None;
	Position			SKTAttack::m_SKTOutdoorPos = Positions::None;
	bool				SKTAttack::m_firstTank = false;

	int SKTAttack::Count(UnitType type)
	{
		return (int)count_if(m_Instances.begin(), m_Instances.end(), [type](const SKTAttack * u) {
			return u->Agent()->Completed() && u->Agent()->Type() == type;
		});
	}


	SKTAttack::SKTAttack(MyUnit * pAgent)
		: Behavior(pAgent, behavior_t::GuardLoc)
	{
		PUSH_BACK_UNCONTAINED_ELEMENT(m_Instances, this);
		if (m_SKTGuardPos == Positions::None) m_SKTGuardPos = findSKTGuard();
		if (m_SKTOutdoorPos == Positions::None) m_SKTOutdoorPos = findSKTOurdoor();
	}



	SKTAttack::~SKTAttack()
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


	void SKTAttack::ChangeState(state_t st)
	{
		CI(this);
		assert_throw(m_state != st);

		m_state = st; OnStateChanged();
	}


	string SKTAttack::StateName() const
	{
		CI(this);
		switch (State())
		{
		case guard:			return "guard";
		case outdoor:		return "outdoor";
		case attack:		return "attack";
		default:			return "?";
		}
	}


	void SKTAttack::OnFrame_v()
	{
		CI(this);

		if (!Agent()->Completed()) return;
		if (!Agent()->CanAcceptCommand()) return;

		bw->drawCircleMap(m_SKTGuardPos, 16, Colors::Red, true);
		bw->drawCircleMap(m_SKTOutdoorPos, 16, Colors::Red, true);
		bw->drawLineMap(m_SKTGuardPos, m_SKTOutdoorPos, Colors::Red);

		// 选择攻击目标
		ChooseAttackTarget();
		if (State() == attack)
		{
			state_t tankState = attack;
			MyUnit * frontTank = nullptr;
			for (auto * u : m_Instances)
			{
				// 攻击模式下的完整坦克
				if (u->Agent()->Completed() && u->State() != guard)
				{
					if (u->Agent()->Type() == Terran_Siege_Tank_Tank_Mode || u->Agent()->Type() == Terran_Siege_Tank_Siege_Mode)
					{
						if (!frontTank ||
							groundDist(u->Agent()->Pos(), me().StartingBase()->Center()) > groundDist(frontTank->Pos(), me().StartingBase()->Center()))
						{
							frontTank = u->Agent();
							tankState = u->State();
						}
					}
				}
			}
			// 攻击模式下，向第一辆坦克移动
			if (frontTank)
			{
				if (Agent()->Type() == Terran_Marine)
				{
					if (Agent()->Pos().getApproxDistance(frontTank->Pos()) > (tankState == outdoor ? 7 : 4) * 32)
					{
						Agent()->Move(frontTank->Pos());
						return;
					}
				}
				// scv，追随并修理
				if (Agent()->Is(Terran_SCV))
				{
					if (Agent()->Pos().getApproxDistance(frontTank->Pos()) > (tankState == outdoor ? 4 : 2) * 32)
					{
						Agent()->Move(frontTank->Pos());
						return;
					}
					if (frontTank->Life() < frontTank->MaxLife() * 0.8)
					{
						Agent()->As<Terran_SCV>()->Repair(frontTank);
						return;
					}
					// 距离太近，停下
					if (Agent()->Pos().getApproxDistance(frontTank->Pos()) <= 1 * 32)
					{
						Agent()->HoldPosition();
					}
				}
				else if (Agent()->Type() == Terran_Vulture)
				{
					if (Agent()->Pos().getApproxDistance(frontTank->Pos()) > 8 * 32)
					{
						Agent()->Move(frontTank->Pos());
						return;
					}
				}
				else if (Agent()->Type() == Terran_Siege_Tank_Tank_Mode && frontTank == this->Agent())
				{
					// 周围必须有农民
					int aroundCount = 0;
					for (auto * u : m_Instances)
					{
						if (u != this && u->Agent()->Pos().getApproxDistance(Agent()->Pos()) < 32 * 7 && u->Agent()->Type() == Terran_SCV)
						{
							++aroundCount;
						}
					}
					if (aroundCount == 0)
					{
						// 周围没有农民，回家
						SmartAttack(me().StartingBase()->Center());
						return;
					}
					if (Agent()->Life() < 40)
					{
						// 停下来待修理
						Agent()->AttackMove(Agent()->Pos());
						return;
					}
				}
			}
			// 处理落单的单位（非坦克）
			if (Agent()->Type() != Terran_Siege_Tank_Tank_Mode && Agent()->Type() != Terran_Siege_Tank_Siege_Mode)
			{
				int xx = Agent()->Pos().x, yy = Agent()->Pos().y, aroundCount = 0;
				for (auto * u : m_Instances)
				{
					if (u != this && u->Agent()->Pos().getApproxDistance(Agent()->Pos()) < 32 * 7)
					{
						xx += u->Agent()->Pos().x;
						yy += u->Agent()->Pos().y;
						++aroundCount;
					}
				}
				// 周围没有单位，并且不止一个单位，回家
				if (aroundCount == 0 && m_Instances.size() > 1)
				{
					SmartAttack(me().StartingBase()->Center());
					return;
				}
			}
		}
		if (State() != attack)
		{
			// 如果已经出基地了，进入attack
			if (ai()->GetMap().GetArea(TilePosition(Agent()->Pos())) != me().StartingBase()->GetArea())
				if (Agent()->Pos().getApproxDistance(me().StartingBase()->Center()) > 32 * 32)
				{
					ChangeState(attack);
				}
		}

		switch (State())
		{
		case guard:		OnFrame_guard(); break;
		case outdoor:	OnFrame_outdoor(); break;
		case attack:	OnFrame_attack(); break;
		default: assert_throw(false);
		}


	}

	void SKTAttack::OnFrame_guard()
	{
		CI(this);
		// 距离太远，移动
		if (Agent()->Pos().getApproxDistance(m_SKTGuardPos) > 32 * 3)
		{
			if (Agent()->Pos().getApproxDistance(m_SKTGuardPos) > 32 && Agent()->Type() == Terran_Siege_Tank_Tank_Mode)
			{
				SmartAttack(m_SKTGuardPos);
			}
			else
			{
				SmartAttack(m_SKTGuardPos);
			}
			return;
		}
		// 坦克没出来，移动
		if (me().CompletedUnits(Terran_Siege_Tank_Tank_Mode) <= 1)
		{
			SmartAttack(m_SKTGuardPos);
			return;
		}
		// 坦克没到位，移动
		if (!m_firstTank && Agent()->Type() != Terran_Siege_Tank_Tank_Mode && find_if(m_Instances.begin(), m_Instances.end(), [](const SKTAttack * u) {
			return u->Agent()->Type() == Terran_Siege_Tank_Tank_Mode && u->State() == outdoor;
		}) == m_Instances.end())
		{
			SmartAttack(m_SKTGuardPos);
			return;
		}
		ChangeState(outdoor);
	}

	void SKTAttack::OnFrame_outdoor()
	{
		CI(this);
		// 记录第一辆坦克
		if (Agent()->Type() == Terran_Siege_Tank_Tank_Mode)
		{
			m_firstTank = true;
		}
		// 距离太远，移动
		if (Agent()->Pos().getApproxDistance(m_SKTOutdoorPos) > 32 * 5)
		{
			SmartAttack(m_SKTOutdoorPos);
			return;
		}
		ChangeState(attack);
	}

	void SKTAttack::OnFrame_attack()
	{
		CI(this);
		if (him().StartingVBase())
		{
			auto pHisMain = him().StartingVBase();
			auto pHisNatural = findNatural(pHisMain);
			// 距离敌人二矿太远，移动
			if (ai()->Frame() > 10800)
			{
				SmartAttack(pHisMain->BWEMPart()->Center());
				return;
			}
			auto hisNaturalPos = pHisNatural->BWEMPart()->Center();
			auto myMainPos = me().StartingVBase()->BWEMPart()->Center();
			int dist = hisNaturalPos.getApproxDistance(myMainPos);
			Position attackPos = (hisNaturalPos * (dist - 3 * 32) + myMainPos * 3 * 32) / dist;
			SmartAttack(attackPos);
			return;
		}
		
	}
	void SKTAttack::OnFrame_raze()
	{
		CI(this);


	}
	void SKTAttack::ChooseAttackTarget()
	{
		CI(this);
		bool inHisNatural = false;
		if (him().StartingVBase())
		{
			auto hisNatural = findNatural(him().StartingVBase());
			auto hisNaturalArea = hisNatural->BWEMPart()->GetArea();
			if (ai()->GetMap().GetArea(TilePosition(Agent()->Pos())) == hisNaturalArea)
			{
				inHisNatural = true;
			}
		}
		multimap<int, HisUnit *> Candidates;
		for (const FaceOff & faceOff : Agent()->FaceOffs())
			if (HisUnit * pTarget = faceOff.His()->IsHisUnit())
				if (faceOff.MyAttack() > 0)
					if (!pTarget->InFog() && pTarget->Unit()->exists())
						//if (!pTarget->Type().isWorker())
						if (!pTarget->Is(Zerg_Egg))
							if (!pTarget->Is(Zerg_Larva))
								if ((pTarget->GroundRange() <= 3 * 32)
									|| (pTarget->Is(Zerg_Hydralisk))
									|| (pTarget->Is(Protoss_Dragoon) && inHisNatural))
								{
									int score = pTarget->LifeWithShields();

									int distScore = faceOff.DistanceToMyRange();
									if (him().IsProtoss())
										if (Agent()->Is(Terran_Marine))
											distScore = distScore * 2 + 50;
									//if (favorWeakest) distScore /= 5;

									score += distScore;

									if (Agent()->Is(Terran_SCV)) score += static_cast<int>(1000 * pTarget->Speed());
									if (pTarget->Is(Protoss_Reaver)) score -= 300;
									if (pTarget->Is(Protoss_Dark_Templar)) score -= 300;

									if (pTarget->Type() == Protoss_Dragoon)
										score -= 1000;
									if (pTarget->Type() == Protoss_Zealot)
										score -= 500;

									Candidates.emplace(score, pTarget);
								}

		if (Candidates.empty()) m_target = nullptr;

		else m_target = Candidates.begin()->second;
		return;
	}

		void SKTAttack::SmartAttack(Position p)
		{
			CI(this);
			if (m_target)
			{
				Agent()->Attack(m_target);
				bw->drawLineMap(Agent()->Pos(), m_target->Pos(), Colors::Blue);
			}
			else
			{
				Agent()->AttackMove(p);
				bw->drawLineMap(Agent()->Pos(), p, Colors::Blue);
			}
		}




} // namespace iron



