//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "guardloc.h"
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

namespace { auto & bw = Broodwar; }


namespace iron
{

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class GuardLoc
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////

	vector<GuardLoc *> GuardLoc::m_Instances;


	GuardLoc::GuardLoc(MyUnit * pAgent, const Position target)
		: Behavior(pAgent, behavior_t::GuardLoc), m_target(target)
	{
		PUSH_BACK_UNCONTAINED_ELEMENT(m_Instances, this);
	}



	GuardLoc::~GuardLoc()
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


	void GuardLoc::ChangeState(state_t st)
	{
		CI(this);
		assert_throw(m_state != st);

		m_state = st; OnStateChanged();
	}


	string GuardLoc::StateName() const
	{
		CI(this);
		switch (State())
		{
		case moving:			return "moving(" + to_string(ai()->Frame() - m_inMovingSince) + ")";
		case guarding:			return "guarding(" + to_string(ai()->Frame() - m_inGuardingSince) + ")";
		default:				return "?";
		}
	}


	void GuardLoc::OnFrame_v()
	{
		CI(this);

		if (!Agent()->Completed()) return;
		if (!Agent()->CanAcceptCommand()) return;

		//如果周围有威胁，进入guarding
		if ((Agent()->Is(Terran_Siege_Tank_Tank_Mode) || Agent()->Is(Terran_Siege_Tank_Siege_Mode)))
			if (me().HasResearched(TechTypes::Tank_Siege_Mode))
			if (!findThreats(Agent(), 6 * 32).empty())
		{
			ChangeState(guarding);
		}

		// 如果站在建筑位置，进入moving
		if (BuildOnAgent())
		{
			ChangeState(moving);
		}

		// 如果放弃Guard
		if (!Agent()->GuardInLoc())
		{
			// 清除最终命令
			Agent()->UnsetLastOrder();
			// 转化为Unsiege
			if (Agent()->Is(Terran_Siege_Tank_Siege_Mode))
			{
				return static_cast<My<Terran_Siege_Tank_Tank_Mode> *>(Agent())->Unsiege();
			}
			// 进入默认状态
			return Agent()->ChangeBehavior<DefaultBehavior>(Agent());
		}

		switch (State())
		{
		case moving:		OnFrame_moving(); break;
		case guarding:		OnFrame_guarding(); break;
		default: assert_throw(false);
		}
	}

	void GuardLoc::OnFrame_moving()
	{
		// 进入guarding时间清零
		m_inGuardingSince = 0;
		// 清除最终命令
		Agent()->UnsetLastOrder();

		// 转化为Unsiege
		if (Agent()->Is(Terran_Siege_Tank_Siege_Mode))
			return static_cast<My<Terran_Siege_Tank_Tank_Mode> *>(Agent())->Unsiege();

		// 如果距离大于6，继续移动
		if (Agent()->Pos().getApproxDistance(m_target) > 6 * 32)
		{
			m_inMovingSince = 0;
			return Agent()->Move(m_target);
		}

		if (m_inMovingSince == 0)
			m_inMovingSince = ai()->Frame();

		// 如果8秒内距离大于3，继续移动
		if (ai()->Frame() - m_inMovingSince < 24 * 8)
			if (Agent()->Pos().getApproxDistance(m_target) > 3 * 32)
				return Agent()->Move(m_target);

		// 如果和导弹塔重合，继续移动
		TilePosition agentLoc = (TilePosition)Agent()->Pos();
		if (ai()->GetVMap().FirstTurretsRoom(ai()->GetMap().GetTile(agentLoc)) ||
			ai()->GetVMap().FirstTurretsRoom(ai()->GetMap().GetTile(agentLoc + TilePosition(1, 0))) ||
			ai()->GetVMap().FirstTurretsRoom(ai()->GetMap().GetTile(agentLoc + TilePosition(0, 1))) ||
			ai()->GetVMap().FirstTurretsRoom(ai()->GetMap().GetTile(agentLoc + TilePosition(1, 1))))
			return Agent()->Move(m_target);

		// 脱离moving，进入guarding
		m_inMovingSince = 0;
		if (me().HasResearched(TechTypes::Tank_Siege_Mode))
			return ChangeState(guarding);
	}

	void GuardLoc::OnFrame_guarding()
	{
		m_inMovingSince = 0;
		if (!Agent()->LastOrder())
		{
			Agent()->SetLastOrder();
			m_inGuardingSince = ai()->Frame();
			// 坦克siege
			if (Agent()->Is(Terran_Siege_Tank_Tank_Mode))
				return static_cast<My<Terran_Siege_Tank_Tank_Mode> *>(Agent())->Siege();
			// 其余单位Hold
			else
				return Agent()->HoldPosition();
		}

		for (const FaceOff & faceOff : Agent()->FaceOffs())
			if (!faceOff.His()->InFog())
				if (faceOff.HisAttack())
					if (!faceOff.His()->Flying())
						if (faceOff.His()->GroundRange() <= 3 * 32 || faceOff.His()->Is(Zerg_Hydralisk)
							)
							if (faceOff.DistanceToHisRange() <
								(faceOff.His()->Type().isWorker() ? 3 * 32 :
									faceOff.His()->Is(Protoss_Dragoon) ? 2 * 32 :
									faceOff.His()->Is(Zerg_Hydralisk) ? 4 * 32 :
									5 * 32))
							{
								int tileRadius = 12;
								vector<MyUnit *> MyUnitsAround = ai()->GetGridMap().GetMyUnits(ai()->GetMap().Crop(TilePosition(Agent()->Pos()) - tileRadius), ai()->GetMap().Crop(TilePosition(Agent()->Pos()) + tileRadius));
								for (MyUnit * u : MyUnitsAround)
									if (u->Is(Terran_Vulture) || u->Is(Terran_Marine) || u->Is(Terran_Goliath))
										if (u->Completed())
											if (!u->Loaded())
												if (!u->GetBehavior()->IsDestroying())
													if (!u->GetBehavior()->IsKillingMine())
														if (!u->GetBehavior()->IsLaying())
															if (!u->GetBehavior()->IsFighting())
																if (!u->GetBehavior()->IsSniping())
																	if (!u->GetBehavior()->IsWalking())
																	{
																		///	ai()->SetDelay(500);
																		///	bw << u->NameWithId() << " helps attacked " << Agent()->NameWithId() << endl;
																		u->ChangeBehavior<Fighting>(u, Agent()->Pos(), zone_t::ground, 1, bool("protectTank"));
																	}
								break;
							}

		auto threats = findThreats(Agent(), 3 * 32);
		// 如果周围没有威胁
		if (threats.empty())
		{
			//如果距离>3
			if (Agent()->Pos().getApproxDistance(m_target) > 3 * 32)
			{
				//进入moving
				return ChangeState(moving);
			}
			// 如果激活TankAdvance，并且30s不动，推进m_target
			if (auto s = ai()->GetStrategy()->Active<TankAdvance>())
				if (!Agent()->GuardInNatural())
					if (ai()->Frame() - m_inGuardingSince > 30 * 24)
					{
						m_inGuardingSince = ai()->Frame();
						m_target = findTankGuard(s->GetFrontLine());
					}
		}

		//// 进入修理状态
		//if (Agent()->Type().isMechanical())
		//	if (Agent()->Life() < Agent()->MaxLife())
		//		if (Agent()->RepairersCount() < Agent()->MaxRepairers())
		//			if (Repairing * pRepairer = Repairing::GetRepairer(Agent(),
		//				(Agent()->Life() * 4 > Agent()->MaxLife() * 3) ? 16 * 32 :
		//				(Agent()->Life() * 4 > Agent()->MaxLife() * 2) ? 32 * 32 :
		//				(Agent()->Life() * 4 > Agent()->MaxLife() * 1) ? 64 * 32 : 1000000))
		//				return Agent()->ChangeBehavior<Repairing>(Agent(), pRepairer);

	}

	bool GuardLoc::BuildOnAgent()
	{
		for (auto & build : Constructing::Instances())
		{
			// 计算建筑上下限
			auto topLeft = build->Location();
			TilePosition dim(build->Type().tileSize());
			// 带附件维度+2
			if (build->Type().canBuildAddon())
				if (build->Type() != Terran_Command_Center)
					dim.x += 2;
			auto bottomRight = topLeft + dim;
			// 计算单位上下限
			auto agentTopLeft = (TilePosition)(Agent()->Pos());
			auto agentBottomRight = agentTopLeft + Agent()->Type().tileSize();
			// 计算是否重合
			for (int x = topLeft.x; x < bottomRight.x; ++x)
				for (int y = topLeft.y; y < bottomRight.y; ++y)
					if (x >= agentTopLeft.x && x < agentBottomRight.x && y >= agentTopLeft.y && y < agentBottomRight.y)
					{
						//bw->sendText("%s build on %s", build->Type().toString().c_str(), Agent()->Type().toString().c_str());
						//bw->drawLineMap(Agent()->Pos(), (Position)build->Location(), Colors::Purple);
						return true;
					}
		}
		return false;
	}



} // namespace iron



