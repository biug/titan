//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "me.h"
#include "production.h"
#include "army.h"
#include "cc.h"
#include "comsat.h"
#include "../behavior/mining.h"
#include "../behavior/constructing.h"
#include "../behavior/defaultBehavior.h"
#include "../behavior/sktAttack.h"
#include "../territory/stronghold.h"
#include "../strategy/strategy.h"
#include "../strategy/walling.h"
#include "../strategy/wraithRush.h"
#include "../strategy/goliathRush.h"
#include "../strategy/zerglingRush.h"
#include "../strategy/shallowTwo.h"
#include "../strategy/stone.h"
#include "../strategy/expand.h"
#include "../strategy/guardNatural.h"
#include "../strategy/tankAdvance.h"
#include "../strategy/skt.h"
#include "../bwem/src/map.h"
#include "../behavior/guardLoc.h"
#include "../units/factory.h"
#include "../interactive.h"
#include "../Iron.h"
#include "../territory/vbase.h"
namespace { auto & bw = Broodwar; }

namespace iron
{


class Me & me() { return ai()->Me(); }

//////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                          //
//                                  class Me
//                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////

	
Me::Me(char)
: m_Production(make_unique<iron::Production>()),
  m_Army(make_unique<iron::Army>())
{
}

	
Me::~Me()
{
}


const BWAPI::Player Me::Player() const
{
	return bw->self();
}


BWAPI::Player Me::Player()
{
	return bw->self();
}

	
void Me::Initialize()
{
	for (auto & base : ai()->GetVMap().Bases())
		if (base.BWEMPart()->Location() == Player()->getStartLocation())
			AddBase(&base);

	assert_throw(Bases().size() == 1);

	AddBaseStronghold(Bases().back());

	m_pNaturalArea = findNatural(me().StartingVBase())->BWEMPart()->GetArea();

	const Area * pStartingArea = me().StartingBase()->GetArea();
	if (const Wall * wall = me().StartingVBase()->GetWall())
	{
		WalkPosition topInnerArea(nearestEmpty(center(wall->InnerArea()->Top())));
		WalkPosition topOuterArea(nearestEmpty(center(wall->OuterArea()->Top())));

		for (bool inSideWall : {false, true})
			ai()->GetMap().BreadthFirstSearch(inSideWall ? topInnerArea : topOuterArea,
				[](const MiniTile & , WalkPosition) { return false; }, // findCond
						
				[wall, inSideWall, pStartingArea](const MiniTile & miniTile, WalkPosition t) // visitCond
					{
						if ((miniTile.AreaId() == wall->OuterArea()->Id()) ||
							(miniTile.AreaId() == wall->InnerArea()->Id()) ||
							(miniTile.AreaId() == pStartingArea->Id()))
							if (!wall->UnderWall(center(t)))
							{
								const Tile & tile = ai()->GetMap().GetTile(TilePosition(t), check_t::no_check);
								if (!tile.GetNeutral())
								{
									if (inSideWall)
										ai()->GetVMap().SetInsideWall(tile);
									else
										ai()->GetVMap().SetOutsideWall(tile);
									return true;
								}
							}
							return false;
					}, false);
	}
}
	
void Me::Update()
{
	for (auto * u : Units())
		exceptionHandler("Me::Update(" + u->NameWithId() + ")", 5000, [u]()
		{
			u->Update();
		});

	for (auto * b : Buildings())
		exceptionHandler("Me::Update(" + b->NameWithId() + ")", 5000, [b]()
		{
			b->Update();
		});

	m_mineralsAvailable = Player()->minerals();
	m_gasAvailable = Player()->gas();

	static vector<int> PreviousSupplyUsed(ai()->Latency() + 2, Player()->supplyUsed() / 2);
	for (int i = (int)PreviousSupplyUsed.size()-1 ; i > 0 ; --i)
		PreviousSupplyUsed[i] = PreviousSupplyUsed[i-1];
	PreviousSupplyUsed[0] = Player()->supplyUsed() / 2;
	bool supplyUsedStable = true;
	for (auto n : PreviousSupplyUsed)
		if (n != PreviousSupplyUsed[0])
			supplyUsedStable = false;
/*
	static vector<int> PreviousSupplyTotal(ai()->Latency() + 2, Player()->supplyTotal() / 2);
	for (int i = (int)PreviousSupplyTotal.size()-1 ; i > 0 ; --i)
		PreviousSupplyTotal[i] = PreviousSupplyTotal[i-1];
	PreviousSupplyTotal[0] = Player()->supplyTotal() / 2;
	bool supplyTotalStable = true;
	for (auto n : PreviousSupplyTotal)
		if (n != PreviousSupplyTotal[0])
			supplyTotalStable = false;
*/
	if (supplyUsedStable) m_supplyUsed = Player()->supplyUsed() / 2;

	//if (supplyTotalStable)
	m_supplyMax = Player()->supplyTotal() / 2;


	m_factoryActivity = 0;
	if (!me().Buildings(Terran_Factory).empty())
	{
		for (const auto & b : me().Buildings(Terran_Factory))
			m_factoryActivity += b->Activity();

		m_factoryActivity /= me().Buildings(Terran_Factory).size();
	}


	m_barrackActivity = 0;
	if (!me().Buildings(Terran_Barracks).empty())
	{
		for (const auto & b : me().Buildings(Terran_Barracks))
			m_barrackActivity += b->Activity();

		m_barrackActivity /= me().Buildings(Terran_Barracks).size();
	}


	m_medicForce = 0.0;
	for (const auto & u : me().Units(Terran_Medic))
		m_medicForce += 0.5 + u->Energy()/100.0;


	m_SCVworkers = count_if(Units(Terran_SCV).begin(), Units(Terran_SCV).end(), [](const unique_ptr<MyUnit> & u){ return u->GetStronghold(); });
	m_SCVsoldiersForever = count_if(Units(Terran_SCV).begin(), Units(Terran_SCV).end(), [](const unique_ptr<MyUnit> & u){ return !u->GetStronghold() && u->IsMy<Terran_SCV>()->SoldierForever(); });
	m_SCVsoldiers = Units(Terran_SCV).size() - m_SCVworkers;
	m_freeTurrets = count_if(Buildings(Terran_Missile_Turret).begin(), Buildings(Terran_Missile_Turret).end(), [](const unique_ptr<MyBuilding> & b){ return !b->GetStronghold(); });

	int freeTanks = count_if(Units(Terran_Siege_Tank_Tank_Mode).begin(), Units(Terran_Siege_Tank_Tank_Mode).end(), [](const unique_ptr<MyUnit> & u){ return u->Completed() && !u->GetStronghold(); });
	int freeGoliaths = count_if(Units(Terran_Goliath).begin(), Units(Terran_Goliath).end(), [](const unique_ptr<MyUnit> & u){ return u->Completed() && !u->GetStronghold(); });


	int a = freeTanks + freeGoliaths + m_freeTurrets;
	int b = CompletedUnits(Terran_Vulture);

/*
	if (him().MayDarkTemplar())
		if (const ShallowTwo * pShallowTwo = ai()->GetStrategy()->Has<ShallowTwo>())
			if (!pShallowTwo->KeepMarinesAtHome())
				b = max(b, CompletedUnits(Terran_Marine)/2);
*/
	int aa = a == 0 ? 0 : 2 + int(0.5 + 2*log((double)(a+1)));
	int bb = int(0.5 + log((double)(b+1)));

	m_SCVsoldiersObjective = aa + bb - (aa && bb ? 1 : 0);

	if (him().IsZerg() && him().MayMuta())
		if (!ai()->GetStrategy()->Detected<ZerglingRush>())
			if (me().Units(Terran_Goliath).empty())
			{
				if (him().AllUnits(Zerg_Mutalisk).size() == 0)
					 if (me().CompletedBuildings(Terran_Armory) == 0)
						m_SCVsoldiersObjective += 2;

				if (him().AllUnits(Zerg_Mutalisk).size() > 0)
					if (me().CompletedBuildings(Terran_Command_Center) < 2)
						if (m_freeTurrets == 0)
							m_SCVsoldiersObjective = 0;

			}

	if (ai()->GetStrategy()->Detected<WraithRush>()) m_SCVsoldiersObjective = 0;
	if (ai()->GetStrategy()->Detected<GoliathRush>()) m_SCVsoldiersObjective = 0;

	if (ai()->GetStrategy()->Active<Walling>() || me().Army().KeepTanksAtHome())
		m_SCVsoldiersObjective = 0;

	if (Interactive::moreSCVs) m_SCVsoldiersObjective = 200;

	if (auto s = ai()->GetStrategy()->Active<Stone>())
		 m_SCVsoldiersObjective = s->SCVsoldiersObjective();


//	m_SCVsoldiersObjective = int(2*sqrt((double)CountMechGroundFighters()) - 0.6);
//	if (Bases().size() <= 2)
//		if (Army().MyGroundUnitsAhead() < Army().HisGroundUnitsAhead())
//			m_SCVsoldiersObjective /= 2;

	UpdateScans();
	Army().Update();

}

/*
a/b    aa  bb
0 : 	0 	0
1 : 	3 	1
2 : 	4 	1
3 : 	5 	1
4 : 	5 	2
5 : 	6 	2
6 : 	6 	2
7 : 	6 	2
8 : 	6 	2
9 : 	7 	2
10 : 	7 	2
11 : 	7 	2
12 : 	7 	3
13 : 	7 	3
14 : 	7 	3
15 : 	8 	3
16 : 	8 	3*/

	
void Me::UpdateScans()
{
	const int cost = Cost(TechTypes::Scanner_Sweep).Energy();

	m_totalAvailableScans = 0;
	int maxAvailableScansPerComsat = 0;
	m_pBestComsat = nullptr;
	for (const unique_ptr<MyBuilding> & b : me().Buildings(Terran_Comsat_Station))
		if (b->Completed())
		{
			int availableScans = b->Energy() / cost;
			m_totalAvailableScans += availableScans;
			if (availableScans > maxAvailableScansPerComsat)
			{
				maxAvailableScansPerComsat = availableScans;
				m_pBestComsat = b->IsMy<Terran_Comsat_Station>();
			}
		}
}


void Me::HandleMacroLoaction(Expert * e, const MacroItem & item)
{
	MacroLocation mloc = item.GetMacroLocation();
	if (mloc == MacroLocation::Natural)
	{
		auto * natural = findNatural(me().StartingVBase());
		if (item.IsBuilding() && item.GetUnitType() == Terran_Command_Center)
		{
			// base加入me
			me().AddBase(natural);
			assert_throw(me().Bases().size() >= 2);
			me().AddBaseStronghold(me().Bases().back());
			natural->SetCreationTime();
			e->SetSpecLocation(natural->BWEMPart()->Location());
		}
		else if (item.IsBuilding() && item.GetUnitType() == Terran_Bunker)
		{
			e->SetSpecLocation((TilePosition)findNaturalGuard(me().StartingVBase(), 32 * 5));
		}
	}
}
	
void Me::RunExperts()
{
	m_opening.DrawTextOnScreen(450);
	if (!m_opening.IsEmpty() && m_opening.FirstItem().IsCommand())
	{
		if (m_opening.FirstItem().GetCommandType().GetType() == MacroCommandType::BarracksRallyNatural)
		{
			if (me().CompletedBuildings(Terran_Barracks) > 0)
			{
				me().Buildings(Terran_Barracks)[0]->Unit()->setRallyPoint(findNatural(me().StartingVBase())->BWEMPart()->Center());
				m_opening.PopItem();
			}
		}
		else if (m_opening.FirstItem().GetCommandType().GetType() == MacroCommandType::KeepTrainSCV)
		{
			m_opening.addKeepProduceItems(Terran_SCV, m_opening.FirstItem().GetCommandType().GetAmount());
			m_opening.PopItem();
		}
		else if (m_opening.FirstItem().GetCommandType().GetType() == MacroCommandType::KeepBuildBarracks)
		{
			m_opening.addKeepProduceItems(Terran_Barracks, m_opening.FirstItem().GetCommandType().GetAmount());
			m_opening.PopItem();
		}
		else if (m_opening.FirstItem().GetCommandType().GetType() == MacroCommandType::KeepBuildDepot)
		{
			m_opening.addKeepProduceItems(Terran_Supply_Depot, m_opening.FirstItem().GetCommandType().GetAmount());
			m_opening.PopItem();
		}
		else if (m_opening.FirstItem().GetCommandType().GetType() == MacroCommandType::KeepTrainMarine)
		{
			m_opening.addKeepProduceItems(Terran_Marine, m_opening.FirstItem().GetCommandType().GetAmount());
			m_opening.PopItem();
		}
		else if (m_opening.FirstItem().GetCommandType().GetType() == MacroCommandType::BuildMachineShop)
		{
			m_opening.SetBuildMachineShop();
			m_opening.PopItem();
		}
	}

	bw->drawTextMap(100, 100, "%d", ai()->Frame());

	//int yy = 40;
	for (auto & e : Production().Experts())
	{
		//if (e->Priority() > 0)
		//{
		//	bw->drawTextScreen(100, yy, "%s %d", e->ToString().c_str(), e->Priority());
		//	yy += 15;
		//}
		if (!e->Unselected())
			exceptionHandler("Me::RunExperts(" + e->ToString() + ")", 5000, [&]()
		{
			// 先处理MacroLocation
			if (!m_opening.IsEmpty() && m_opening.CheckRightExpert(e) && !e->Started())
			{
				HandleMacroLoaction(e, m_opening.FirstItem());
			}
			e->OnFrame();
			if (!m_opening.IsEmpty() && m_opening.CheckRightExpert(e))
			{
				if (e->Unselected())
				{
					m_opening.PopItem();
					m_opening.SetPreWalking(false);
					e->SetSpecLocation(TilePositions::None);
				}
			}
			else if (!m_opening.IsAllEmpty() && m_opening.CheckRightKeepExpert(e))
			{
				if (e->Unselected())
				{
					m_opening.finishOneItem(e->GetID());
					e->SetSpecLocation(TilePositions::None);
				}
			}
		});
	}
}

	
void Me::RunBehaviors()
{
	for (auto & u : Units(Terran_Vulture_Spider_Mine))
		exceptionHandler("Me::RunBehaviors(" + u->NameWithId() + ")", 5000, [&u]()
		{
			u->GetBehavior()->OnFrame();
		});

	auto * sTankGuard = ai()->GetStrategy()->Active<TankAdvance>();
	Position tankGuardPos = Positions::None;
	if (sTankGuard)
		tankGuardPos = findTankGuard(sTankGuard->GetFrontLine());
	My<Terran_Siege_Tank_Tank_Mode>::m_tankGuardNatural = 0;

	int vultureGuard = 0;
	for (auto & u : Units(Terran_Vulture))
		if (u->GuardInLoc())
			vultureGuard += 1;

	// 设置SKT开局
	if (!m_onceSKT && ai()->GetStrategy()->Active<SKT>())
	{
		m_onceSKT = true;
	}
	// 设置DT防下
	if (!m_DTRushByeBye)
	{
		if (him().LostUnits(Protoss_Shuttle) == 1 && him().Units(Protoss_Dark_Templar).size() == 0)
		{
			m_DTRushByeBye = true;
		}
		if (him().LostUnits(Protoss_Dark_Templar) >= 4)
		{
			m_DTRushByeBye = true;
		}
		if (bw->getFrameCount() > 12000)
		{
			m_DTRushByeBye = true;
		}
	}

	bool onceSKT = m_onceSKT;
	bool dtByebye = m_DTRushByeBye;
	for (auto * u : Units())
		exceptionHandler("Me::RunBehaviors(" + u->NameWithId() + ")", 5000, [u, sTankGuard, tankGuardPos, onceSKT, dtByebye, &vultureGuard]()
			{
				// SKT战术激活时，雷车不转入随机
				if (ai()->GetStrategy()->Active<SKT>())
				{
					if (u->SKTRush() && !u->GetBehavior()->IsSKTAttack())
					{
						u->ChangeBehavior<SKTAttack>(u);
					}
					if (SKTAttack::Count(Terran_Siege_Tank_Tank_Mode) && SKTAttack::Count(Terran_SCV) < 2 && u->Type() == Terran_SCV && !u->GetBehavior()->IsSKTAttack())
					{
						u->SetSKTRush();
						u->ChangeBehavior<SKTAttack>(u);
					}
				}
				// 如果战术不激活，取消该模式
				else
				{
					if (u->SKTRush())
					{
						u->SetGuardInLoc();
						auto hisMain = him().StartingVBase();
						if (hisMain)
						{
							if (!u->GetBehavior()->IsGuardLoc())
							{
								u->ChangeBehavior<GuardLoc>(u, hisMain->BWEMPart()->Center());
							}
						}
						u->UnsetSKTRush();
						if (dtByebye)
						{
							if (auto s = u->GetBehavior()->IsGuardLoc())
							{
								if (s->Target() != hisMain->BWEMPart()->Center())
								{
									u->ChangeBehavior<GuardLoc>(u, hisMain->BWEMPart()->Center());
								}
							}
						}
					}
				}
				// 需要守卫分矿时，保卫分矿
				if (!u->GuardInLoc())
				if (ai()->GetStrategy()->Active<GuardNatural>())
				if (u->Is(Terran_Siege_Tank_Tank_Mode) || u->Is(Terran_Siege_Tank_Siege_Mode))
				{
					u->SetGuardInLoc();
					Position guardPos =
						me().CompletedUnits(Terran_Siege_Tank_Tank_Mode) + me().CompletedUnits(Terran_Siege_Tank_Siege_Mode) > 6 ?
						findNaturalThirdGuard(me().StartingVBase()) :
						findNaturalGuard(me().StartingVBase(), 6 * 32);
					u->ChangeBehavior<GuardLoc>(u, guardPos);
				}

				// 需要推进时，推进
				if (sTankGuard)
					if (u->Is(Terran_Siege_Tank_Tank_Mode) || u->Is(Terran_Siege_Tank_Siege_Mode))
					{
						// 对于不守家的坦克
						if (!u->GuardInNatural())
						{
							if (!u->GuardInLoc())
								u->SetGuardInLoc();
							if (sTankGuard->FindBackwardTanks(u))
							{
								if (auto * b = u->GetBehavior()->IsGuardLoc())
									if (b->Target() != tankGuardPos)
										u->ChangeBehavior<GuardLoc>(u, tankGuardPos);
							}
							if (!u->GetBehavior()->IsGuardLoc()) u->ChangeBehavior<GuardLoc>(u, tankGuardPos);
						}
						// 统计守家坦克数量
						else
						{
							My<Terran_Siege_Tank_Tank_Mode>::m_tankGuardNatural += 1;
						}
					}

				// 雷车回家
				if (sTankGuard && me().CompletedUnits(Terran_Dropship) > 0)
				{
					if (u->Is(Terran_Vulture) && u->Completed())
						if (!u->GuardInLoc() && vultureGuard < 4)
						{
							u->SetGuardInLoc();
							vultureGuard += 1;
							u->ChangeBehavior<GuardLoc>(u, me().StartingBase()->Center());
						}
				}

				if (!u->Is(Terran_Vulture_Spider_Mine))
				if (!u->Unit()->isLockedDown())
				if (!u->Unit()->isMaelstrommed())
				if (!u->Unit()->isStasised())
					u->GetBehavior()->OnFrame();
				
				if (auto * g = u->GetBehavior()->IsGuardLoc())
					bw->drawLineMap(u->Pos(), g->Target(), Colors::Red);
			});

	for (auto * b : Buildings())
		exceptionHandler("Me::RunBehaviors(" + b->NameWithId() + ")", 5000, [b]()
		{
			b->GetBehavior()->OnFrame();
		});

	//auto wall = StartingVBase()->GetWall();
	//auto mainArea = StartingVBase()->GetArea()->BWEMPart();

	//int i1 = mainArea->TopLeft().x, j1 = mainArea->TopLeft().y, i2 = mainArea->BottomRight().x, j2 = mainArea->BottomRight().y;
	//// 找内部点
	//set<pair<int, int>> innerTiles;
	//for (int j = j1; j <= j2; ++j)
	//	for (int i = i1; i <= i2; ++i)
	//	{
	//		Position target = Position(TilePosition(i, j)) + Position(16, 16);
	//		CHECK_POS(target);
	//		if (ai()->GetMap().GetArea(WalkPosition(target)) == mainArea)
	//		{
	//			// 如果有墙，必须在墙内
	//			if (wall)
	//			{
	//				if (ai()->GetVMap().InsideWall(ai()->GetMap().GetTile(TilePosition(target))))
	//					innerTiles.insert(make_pair(i, j));
	//			}
	//			else
	//				innerTiles.insert(make_pair(i, j));
	//		}
	//	}
	//// 找边界
	//set<pair<int, int>> borderTiles;
	//for (auto & tile : innerTiles)
	//	if (innerTiles.find(make_pair(tile.first - 1, tile.second)) == innerTiles.end() ||
	//		innerTiles.find(make_pair(tile.first + 1, tile.second)) == innerTiles.end() ||
	//		innerTiles.find(make_pair(tile.first, tile.second - 1)) == innerTiles.end() ||
	//		innerTiles.find(make_pair(tile.first, tile.second + 1)) == innerTiles.end())
	//		if (mainArea->Geysers().empty() || mainArea->Geysers()[0]->Pos().getApproxDistance(Position(tile.first * 32, tile.second * 32)) > 5 * 32)
	//			bw->drawCircleMap(tile.first * 32, tile.second * 32, 16, Colors::Red);

}

	
void Me::RunProduction()
{
	Production().OnFrame(m_opening);
}


VArea * Me::GetVArea()
{
	return ext(GetArea());
}


void Me::AddUnit(BWAPI::Unit u)
{
	assert_throw(u->getPlayer() == Player());
	assert_throw(!u->getType().isBuilding());
	assert_throw(u->getType() != Terran_Siege_Tank_Siege_Mode);

	vector<unique_ptr<MyUnit>> & List = Units(u->getType());
	assert_throw(none_of(List.begin(), List.end(), [u](const unique_ptr<MyUnit> & up){ return up->Unit() == u; }));

	if (u->getType().isSpell()) return;

	List.push_back(MyUnit::Make(u));
	++m_creationCount[u->getType()];
	List.back()->InitializeStronghold();
	PUSH_BACK_UNCONTAINED_ELEMENT(m_AllUnits, List.back().get());
}


void Me::SetSiegeModeType(BWAPI::Unit u)
{
	vector<unique_ptr<MyUnit>> & List = Units(Terran_Siege_Tank_Tank_Mode);
	auto iMyUnit = find_if(List.begin(), List.end(), [u](const unique_ptr<MyUnit> & up){ return up->Unit() == u; });
	assert_throw(iMyUnit != List.end());

	(*iMyUnit)->SetSiegeModeType();
}


void Me::SetTankModeType(BWAPI::Unit u)
{
	vector<unique_ptr<MyUnit>> & List = Units(Terran_Siege_Tank_Tank_Mode);
	auto iMyUnit = find_if(List.begin(), List.end(), [u](const unique_ptr<MyUnit> & up){ return up->Unit() == u; });
	assert_throw(iMyUnit != List.end());

	(*iMyUnit)->SetTankModeType();
}


void Me::OnBWAPIUnitDestroyed_InformOthers(BWAPIUnit * pBWAPIUnit)
{
	for (auto * u : Units())
		if (u != pBWAPIUnit)
			u->OnOtherBWAPIUnitDestroyed(pBWAPIUnit);

	for (auto * b : Buildings())
		if (b != pBWAPIUnit)
			b->OnOtherBWAPIUnitDestroyed(pBWAPIUnit);
}


void Me::RemoveUnit(BWAPI::Unit u)
{
	UnitType type = u->getType();
	if (type == Terran_Siege_Tank_Siege_Mode) type = Terran_Siege_Tank_Tank_Mode;

	if (type.isSpell()) return;

	vector<unique_ptr<MyUnit>> & List = Units(type);
	auto iMyUnit = find_if(List.begin(), List.end(), [u](const unique_ptr<MyUnit> & up){ return up->Unit() == u; });
	assert_throw(iMyUnit != List.end());

	OnBWAPIUnitDestroyed_InformOthers(iMyUnit->get());
	ai()->GetStrategy()->OnBWAPIUnitDestroyed(iMyUnit->get());

	iMyUnit->get()->GetBehavior()->OnAgentBeingDestroyed();
	iMyUnit->get()->ChangeBehavior<DefaultBehavior>(iMyUnit->get());
	iMyUnit->get()->GetBehavior()->OnAgentBeingDestroyed();
	if (iMyUnit->get()->GetStronghold()) iMyUnit->get()->LeaveStronghold();

	{
		MyUnit * pMyUnit = iMyUnit->get();
		auto i = find(m_AllUnits.begin(), m_AllUnits.end(), pMyUnit);
		assert_throw(i != m_AllUnits.end());
		fast_erase(m_AllUnits, distance(m_AllUnits.begin(), i));
	}

	fast_erase(List, distance(List.begin(), iMyUnit));
	++m_lostUnits[u->getType()];
}


int Me::CompletedBuildings(UnitType type) const
{
	assert_throw(type <= max_utid_used);
	assert_throw(type.isBuilding());
	
	return count_if(Buildings(type).begin(), Buildings(type).end(),
					[](const unique_ptr<MyBuilding> & b){ return b->Completed(); });
}


int Me::BuildingsBeingTrained(UnitType type) const
{
	assert_throw(type <= max_utid_used);
	assert_throw(type.isBuilding());
	
	return (int)Buildings(type).size() - CompletedBuildings(type);
}


int Me::CompletedUnits(UnitType type) const
{
	assert_throw(type <= max_utid_used);
	assert_throw(!type.isBuilding());
	
	return count_if(Units(type).begin(), Units(type).end(),
					[](const unique_ptr<MyUnit> & u){ return u->Completed(); });
}


int Me::UnitsBeingTrained(UnitType type) const
{
	assert_throw(type <= max_utid_used);
	assert_throw(!type.isBuilding());
	
	return (int)Units(type).size() - CompletedUnits(type);
}


void Me::AddBuilding(BWAPI::Unit u)
{
	assert_throw(u->getPlayer() == Player());
	assert_throw(u->getType().isBuilding());

	vector<unique_ptr<MyBuilding>> & List = Buildings(u->getType());
	assert_throw(none_of(List.begin(), List.end(), [u](const unique_ptr<MyBuilding> & up){ return up->Unit() == u; }));

	List.push_back(MyBuilding::Make(u));
	++m_creationCount[u->getType()];
	List.back()->InitializeStronghold();
	PUSH_BACK_UNCONTAINED_ELEMENT(m_AllBuildings, List.back().get());

	List.back()->RecordTrainingExperts(Production());
	List.back()->RecordResearchingExperts(Production());
}


void Me::RemoveBuilding(BWAPI::Unit u, check_t checkMode)
{
	UnitType type = u->getType();
	if (type == Resource_Vespene_Geyser) type = Race().getRefinery();

	vector<unique_ptr<MyBuilding>> & List = Buildings(type);
	auto iMyBuilding = find_if(List.begin(), List.end(), [u](const unique_ptr<MyBuilding> & up){ return up->Unit() == u; });
	if (iMyBuilding != List.end())
	{
		OnBWAPIUnitDestroyed_InformOthers(iMyBuilding->get());

		iMyBuilding->get()->GetBehavior()->OnAgentBeingDestroyed();
		iMyBuilding->get()->ChangeBehavior<DefaultBehavior>(iMyBuilding->get());
		iMyBuilding->get()->GetBehavior()->OnAgentBeingDestroyed();
		if (iMyBuilding->get()->GetStronghold()) iMyBuilding->get()->LeaveStronghold();

		{
			MyBuilding * pMyBuilding = iMyBuilding->get();
			auto i = find(m_AllBuildings.begin(), m_AllBuildings.end(), pMyBuilding);
			assert_throw(i != m_AllBuildings.end());
			fast_erase(m_AllBuildings, distance(m_AllBuildings.begin(), i));
		}

		fast_erase(List, distance(List.begin(), iMyBuilding));
		++m_lostUnits[u->getType()];
	}
	else
	{
		assert_throw(checkMode == check_t::no_check);
	}

	if (u->getType() == Terran_Command_Center)
	{
		for (auto & base : ai()->GetVMap().Bases())
			if (base.BWEMPart()->Center().getApproxDistance(u->getPosition()) < 6 * 32)
			{
				RemoveBase(&base);
				for (int i = 0; i < m_Strongholds.size(); ++i)
					if (!m_Strongholds[i]->Buildings().empty() && m_Strongholds[i]->Buildings()[0]->Pos().getApproxDistance(u->getPosition()) < 6 * 32)
					{
						fast_erase(m_Strongholds, i);
						break;
					}
				bw->sendText("a base delete");
				break;
			}
	}
}


void Me::AddBaseStronghold(VBase * b)
{
	assert_throw(!b->GetStronghold());

	m_Strongholds.push_back(make_unique<BaseStronghold>(b));
}


void Me::AddBase(VBase * b)
{
	PUSH_BACK_UNCONTAINED_ELEMENT(m_Bases, b);
}


void Me::SetLastBase(const VBase * b)
{
	assert_throw(b->Lost());
	auto iBase = find(m_Bases.begin(), m_Bases.end(), b);
	swap(*iBase, m_Bases.back());
}


void Me::RemoveBase(VBase * b)
{
	auto iBase = find(m_Bases.begin(), m_Bases.end(), b);
	assert_throw(iBase != m_Bases.end());

	fast_erase(m_Bases, distance(m_Bases.begin(), iBase));
}


bool Me::CanPay(const Cost & cost) const
{
	if (cost.Minerals() > MineralsAvailable()) return false;
	if (cost.Gas() > GasAvailable()) return false;
	if (cost.Supply() > max(0, SupplyAvailable())) return false;
	return true;
}


int Me::CountMechGroundFighters() const
{
	return	Units(Terran_Vulture).size() +
			Units(Terran_Siege_Tank_Tank_Mode).size() +
			Units(Terran_Goliath).size();
}


vector<const Area *> Me::EnlargedAreas() const
{
	vector<const Area *> List;

	for (VBase * base : Bases())
		List.insert(List.end(), base->GetArea()->EnlargedArea().begin(), base->GetArea()->EnlargedArea().end());

	return List;
}


MyBuilding * Me::FindBuildingNeedingBuilder(UnitType type, bool inStrongHold) const
{
	for (auto & b : Buildings(type))
		if (!b->Completed())
			if (none_of(Constructing::Instances().begin(), Constructing::Instances().end(),
						[&b](const Constructing * c) { return c->Location() == b->TopLeft(); }))
				if ((b->GetStronghold() != nullptr) == inStrongHold)
				{
					if (ai()->GetStrategy()->Active<Stone>())
					{
						if (!b->CanAcceptCommand()) return nullptr;

						static map<MyBuilding *, frame_t > BlockedSince;
						if (BlockedSince.find(b.get()) == BlockedSince.end())
							BlockedSince[b.get()] = ai()->Frame();
						else
							if (ai()->Frame() - BlockedSince[b.get()] > 300)
							{
								BlockedSince.erase(b.get());
								b->CancelConstruction();
							///	bw << "FindBuildingNeedingBuilder : CancelConstruction of " << b->NameWithId() << endl;
								return nullptr;
							}
					}

					return b.get();
				}

	return nullptr;
}


bool Me::TechAvailableIn(TechType techType, frame_t time) const
{
	if (HasResearched(techType)) return true;

	for (UnitType t : {Terran_Academy, Terran_Science_Facility, Terran_Control_Tower, Terran_Physics_Lab, Terran_Covert_Ops, Terran_Machine_Shop})
		for (const auto & b : Buildings(t))
			if (b->Unit()->getTech() == techType)
				return b->TimeToResearch() <= time;

	return false;
}


bool Me::TechAvailableIn(UpgradeType upgradeType, frame_t time) const
{
	if (HasUpgraded(upgradeType)) return true;

	for (UnitType t : {Terran_Academy, Terran_Science_Facility, Terran_Control_Tower, Terran_Physics_Lab, Terran_Covert_Ops, Terran_Armory, Terran_Engineering_Bay})
		for (const auto & b : Buildings(t))
			if (b->Unit()->getUpgrade() == upgradeType)
				return b->TimeToResearch() <= time;

	return false;
}

void Me::SetOpening(string name)
{
	m_opening = GetOpeningByName(name);
}



} // namespace iron



