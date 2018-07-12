//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "expand.h"
#include "strategy.h"
#include "walling.h"
#include "zerglingRush.h"
#include "zealotRush.h"
#include "dragoonRush.h"
#include "marineRush.h"
#include "cannonRush.h"
#include "wraithRush.h"
#include "goliathRush.h"
#include "baseDefense.h"
#include "shallowTwo.h"
#include "terranFastExpand.h"
#include "protossGreedy.h"
#include "massHydra.h"
#include "stone.h"
#include "../units/cc.h"
#include "../units/army.h"
#include "../behavior/mining.h"
#include "../behavior/exploring.h"
#include "../behavior/constructing.h"
#include "../behavior/defaultBehavior.h"
#include "../behavior/sieging.h"
#include "../units/factory.h"
#include "../territory/stronghold.h"
#include "../territory/vgridMap.h"
#include "../interactive.h"
#include "../Iron.h"

namespace { auto & bw = Broodwar; }




namespace iron
{


VBase * findNatural(const VBase * pMain)
{
	assert_throw(pMain);
	VBase * natural = nullptr;

	int minLength = numeric_limits<int>::max();
	for (VBase & base : ai()->GetVMap().Bases())
		if (&base != pMain)
			if (!base.BWEMPart()->Minerals().empty() && !base.BWEMPart()->Geysers().empty())
			{
				int length;
				ai()->GetMap().GetPath(base.BWEMPart()->Center(), pMain->BWEMPart()->Center(), &length);
				if ((length > 0) && (length < minLength))
				{
					minLength = length;
					natural = &base;
				}
			}

	return natural;
}

VBase * findThird(const VBase * pMain)
{
	assert_throw(pMain);
	VBase * pNatural = findNatural(pMain);
	VBase * third = nullptr;
	int minLength = numeric_limits<int>::max();
	for (VBase & base : ai()->GetVMap().Bases())
		if (&base != pMain && &base != pNatural)
			if (!base.BWEMPart()->Minerals().empty() && !base.BWEMPart()->Geysers().empty())
			{
				int length;
				ai()->GetMap().GetPath(base.BWEMPart()->Center(), pMain->BWEMPart()->Center(), &length);
				if ((length > 0) && (length < minLength))
				{
					minLength = length;
					third = &base;
				}
			}

	return third;
}
VBase * findRealThird(const VBase * pMain)
{
	assert_throw(pMain);
	VBase * pNatural = findNatural(pMain);
	VBase * third = nullptr;
	int minLength = numeric_limits<int>::max();
	for (VBase & base : ai()->GetVMap().Bases())
		if (&base != pMain && &base != pNatural)
			if (!base.BWEMPart()->Minerals().empty())
			{
				int length = base.BWEMPart()->Center().getApproxDistance(pMain->BWEMPart()->Center());
				if ((length > 0) && (length < minLength))
				{
					minLength = length;
					third = &base;
				}
			}

	return third;
}

Position findMainGuard(const VBase * pMain)
{
	assert_throw(pMain);
	int xx = 0, yy = 0;
	for (auto & mineral : pMain->BWEMPart()->Minerals())
	{
		xx += mineral->Pos().x;
		yy += mineral->Pos().y;
	}
	return (pMain->BWEMPart()->Center() + Position(xx / pMain->BWEMPart()->Minerals().size(), yy / pMain->BWEMPart()->Minerals().size())) / 2;
}

Position findUplandGuard(const VBase * pMain)
{
	// 遍历base area的点
	// 找到能防守分矿的点

	auto wall = pMain->GetWall();
	auto mainArea = pMain->GetArea()->BWEMPart();
	Position mainCenter = pMain->BWEMPart()->Center();
	Position naturalCenter = findNatural(pMain)->BWEMPart()->Center();

	int i1 = mainArea->TopLeft().x, j1 = mainArea->TopLeft().y, i2 = mainArea->BottomRight().x, j2 = mainArea->BottomRight().y;

	Position bestGuard = Positions::None;

	// 找内部点
	set<pair<int, int>> innerTiles;
	for (int j = j1; j <= j2; ++j)
		for (int i = i1; i <= i2; ++i)
		{
			Position target = Position(TilePosition(i, j)) + Position(16, 16);
			CHECK_POS(target);
			if (ai()->GetMap().GetArea(WalkPosition(target)) == mainArea)
			{
				// 如果有墙，必须在墙内
				if (wall)
				{
					if (ai()->GetVMap().InsideWall(ai()->GetMap().GetTile(TilePosition(target))))
						innerTiles.insert(make_pair(i, j));
				}
				else
					innerTiles.insert(make_pair(i, j));
			}
		}
	// 找边界
	set<pair<int, int>> borderTiles;
	for (auto & tile : innerTiles)
		if (innerTiles.find(make_pair(tile.first - 1, tile.second)) == innerTiles.end() ||
			innerTiles.find(make_pair(tile.first + 1, tile.second)) == innerTiles.end() ||
			innerTiles.find(make_pair(tile.first, tile.second - 1)) == innerTiles.end() ||
			innerTiles.find(make_pair(tile.first, tile.second + 1)) == innerTiles.end())
		if (mainArea->Geysers().empty() || mainArea->Geysers()[0]->Pos().getApproxDistance(Position(tile.first * 32, tile.second * 32)) > 5 * 32)
			borderTiles.insert(tile);

	// 找防守位置
	for (auto & tile : innerTiles)
	{
		Position target = Position(TilePosition(tile.first, tile.second)) + Position(16, 16);

		// 距分矿距离小于18
		int dist = (int)target.getDistance(naturalCenter);
		if (dist >= 30 * 32)
			continue;

		// 不能距边界太近
		bool nearBorder = false;
		for (auto & border : borderTiles)
			if (target.getApproxDistance(Position(border.first * 32, border.second * 32)) < 3 * 32)
				nearBorder = true;
		if (nearBorder) continue;

		// 离基地尽可能近
		if (bestGuard == Positions::None ||
			naturalCenter.getDistance(target) < naturalCenter.getDistance(bestGuard))
			bestGuard = target;
	}

	bw->sendText("best guard is (%d,%d)", bestGuard.x / 32, bestGuard.y / 32);
	return bestGuard;
}

Position findNaturalGuard(const VBase * pMain, int distToNatural)
{
	auto natural = findNatural(pMain);

	Position bestChoke = Positions::None;
	Position baseCenter = me().StartingBase()->Center();
	int minDistance = 0;
	for (auto & choke : natural->GetArea()->BWEMPart()->ChokePoints())
	{
		if (choke->GetAreas().first == me().StartingBase()->GetArea() || choke->GetAreas().second == me().StartingBase()->GetArea())
		{
			continue;
		}
		else
		{
			int dist = (int)baseCenter.getDistance((Position)choke->Center());
			if (minDistance < dist)
			{
				minDistance = dist;
				bestChoke = (Position)choke->Center();
			}
		}
	}
	if (bestChoke != Positions::None)
	{
		Position naturalCenter = natural->BWEMPart()->Center();
		int naturalToChoke = (int)naturalCenter.getDistance(bestChoke);
		Position bunkerPos = bestChoke;
		if (naturalToChoke > distToNatural)
			bunkerPos = (naturalCenter * (naturalToChoke - distToNatural) + bestChoke * distToNatural) / naturalToChoke;
		return bunkerPos;
	}

	return Positions::None;
}


Position findNaturalThirdGuard(const VBase * pMain)
{
	auto natural = findNatural(pMain);
	auto third = findRealThird(pMain);
	int naturalToThird = natural->BWEMPart()->Center().getApproxDistance(third->BWEMPart()->Center()) / 2;

	int length = -1;
	auto & path = ai()->GetMap().GetPath(natural->BWEMPart()->Center(), third->BWEMPart()->Center(), &length);
	if (length < 0) return findNaturalGuard(pMain, 200);
	vector<Position> points;
	points.emplace_back(natural->BWEMPart()->Center());
	for (auto & choke : path)
		points.emplace_back((Position)choke->Center());
	points.emplace_back(third->BWEMPart()->Center());
	for (int i = 0; i < points.size() - 1; ++i)
	{
		int dist = points[i].getApproxDistance(points[i + 1]);
		if (naturalToThird < dist)
		{
			return (points[i] * (dist - naturalToThird) + points[i + 1] * naturalToThird) / dist;
		}
		else
			naturalToThird -= dist;
	}
	return findNaturalGuard(pMain, 200);
}

Position findTankGuard(Position frontline)
{
	if (him().StartingBase() && frontline != Positions::None)
	{
		int length;
		auto & path = ai()->GetMap().GetPath(frontline, him().StartingBase()->Center(), &length);
		if (length < 0) return frontline;
		if (path.size() == 0) return him().StartingBase()->Center();
		Position chokePos = (Position)path[0]->Center();
		int lineToChoke = frontline.getApproxDistance(chokePos);
		if (lineToChoke == 0)
		{
			chokePos = path.size() > 1 ? (Position)path[1]->Center() : him().StartingBase()->Center();
			lineToChoke = frontline.getApproxDistance(chokePos);
		}
		assert_throw(lineToChoke > 0);
		// 如果距下一个路口>3，前进3
		if (lineToChoke > 3 * 32)
			return (frontline * (lineToChoke - 3 * 32) + chokePos * 3 * 32) / lineToChoke;
		// 否则前进4
		else
			return (chokePos * 4 * 32 - frontline * (4 * 32 - lineToChoke)) / lineToChoke;
	}
	return frontline;
}

Position findSKTGuard()
{
	// 遍历base area的点
	// 找到能防守分矿的点

	auto pMain = me().StartingVBase()->BWEMPart();
	auto pNatural = findNatural(me().StartingVBase())->BWEMPart();
	auto wall = me().GetVBase(0)->GetWall();
	auto mainArea = pMain->GetArea();

	int i1 = mainArea->TopLeft().x, j1 = mainArea->TopLeft().y, i2 = mainArea->BottomRight().x, j2 = mainArea->BottomRight().y;

	Position bestGuard = Positions::None;
	
	// 找路口
	Position chokePos = Positions::None;
	for (auto & choke : pMain->GetArea()->ChokePoints())
	{
		if (choke->GetAreas().first == pNatural->GetArea() || choke->GetAreas().second == pNatural->GetArea())
		{
			chokePos = (Position)choke->Center();
			break;
		}
	}
	// 找兵营位置
	Position barracksPos = wall ? (Position)(wall->Locations()[0] + UnitType(Terran_Barracks).tileSize() / 2) : chokePos;
	// 没有墙
	int chokeToBarracks = chokePos.getApproxDistance(barracksPos);
	if (chokeToBarracks == 0) return chokePos;
	// 找内部点
	set<pair<int, int>> innerTiles;
	for (int j = j1; j <= j2; ++j)
		for (int i = i1; i <= i2; ++i)
		{
			Position target = Position(TilePosition(i, j));
			CHECK_POS(target);
			if (ai()->GetMap().GetArea(WalkPosition(target)) == mainArea)
			{
				// 如果有墙，必须在墙内
				if (!wall || ai()->GetVMap().InsideWall(ai()->GetMap().GetTile(TilePosition(target))))
				{
					innerTiles.insert(make_pair(i, j));
				}
			}
		}
	// 找边界
	set<pair<int, int>> borderTiles;
	for (auto & tile : innerTiles)
		// 过滤掉内部点
		if (innerTiles.find(make_pair(tile.first - 1, tile.second)) == innerTiles.end() ||
			innerTiles.find(make_pair(tile.first + 1, tile.second)) == innerTiles.end() ||
			innerTiles.find(make_pair(tile.first, tile.second - 1)) == innerTiles.end() ||
			innerTiles.find(make_pair(tile.first, tile.second + 1)) == innerTiles.end())
			if (mainArea->Geysers().empty() || mainArea->Geysers()[0]->Pos().getApproxDistance(Position(tile.first * 32, tile.second * 32)) > 5 * 32)
			{
				borderTiles.insert(make_pair(tile.first, tile.second));
			}
	for (int d = 2; d <= 5; ++d)
	{
		bestGuard = (barracksPos * (chokeToBarracks + d * 32) - chokePos * d * 32) / chokeToBarracks;
		// 不在边界内
		if (borderTiles.find(make_pair(bestGuard.x / 32, bestGuard.y / 32)) == borderTiles.end())
		{
			break;
		}
	}
	return bestGuard;
}

Position findSKTOurdoor()
{
	auto * natural = findNatural(me().StartingVBase());
	if (natural)
	{
		return natural->BWEMPart()->Center();
	}
	return Positions::None;
}

TilePosition findSKTBarracksLand()
{
	auto wall = me().GetVBase(0)->GetWall();
	auto pNatural = findNatural(me().StartingVBase())->BWEMPart();
	if (!wall) return me().CompletedUnits(Terran_Barracks) > 0 ? (TilePosition)me().Units(Terran_Barracks)[0]->Pos() : TilePositions::None;
	// 找基地
	Position basePos = me().StartingBase()->Center();
	// 后退五格
	Position barracksPos = (Position)(wall->Locations()[0] + UnitType(Terran_Barracks).tileSize() / 2);
	int chokeToBarracks = basePos.getApproxDistance(barracksPos);
	TilePosition landPos = (TilePosition)((barracksPos * (chokeToBarracks - 8 * 32) + basePos * 8 * 32) / chokeToBarracks);
	for (int x = landPos.x - 4; x <= landPos.x + 4; ++x)
	{
		for (int y = landPos.y - 4; y <= landPos.y + 4; ++y)
		{
			if (x != barracksPos.x / 32 || y != barracksPos.y / 32)
			{
				if (rawCanBuild(Terran_Barracks, TilePosition(x, y)))
				{
					return TilePosition(x, y);
				}
			}
		}
	}
	return landPos;
	
}

VBase * findNewBase()
{
	vector<pair<VBase *, int>> Candidates;

	for (VBase & base : ai()->GetVMap().Bases())
		if (!contains(me().Bases(), &base))
		if (him().StartingVBase() != &base)
		{
			TilePosition baseCenter = base.BWEMPart()->Location() + UnitType(Terran_Command_Center).tileSize()/2;
			auto & Cell = ai()->GetGridMap().GetCell(baseCenter);
			if (Cell.HisUnits.empty())
			if (Cell.HisBuildings.empty())
				if (all_of(him().Buildings().begin(), him().Buildings().end(), [baseCenter](const unique_ptr<HisBuilding> & b)
								{ return dist(b->Pos(), center(baseCenter)) > 10*32; }))
					if (!base.BWEMPart()->Minerals().empty())// && !base.BWEMPart()->Geysers().empty())
					{
						int distToMyMain;
						ai()->GetMap().GetPath(base.BWEMPart()->Center(), me().StartingBase()->Center(), &distToMyMain);

						int distToHisMain = 1000000;
						if (him().StartingBase())
							ai()->GetMap().GetPath(base.BWEMPart()->Center(), him().StartingBase()->Center(), &distToHisMain);

						if (distToMyMain > 0)
						{
							int ressourcesScore = base.MineralsAmount() + base.GasAmount();
							int score = ressourcesScore/50 + distToHisMain - distToMyMain;

							if (ai()->GetStrategy()->Active<Stone>())
								score = base.MineralsAmount()/100 + 2*distToHisMain + distToMyMain;

							Candidates.emplace_back(&base, score);
						}
					}
		}

	if (Candidates.empty()) return nullptr;

	sort(Candidates.begin(), Candidates.end(),
		[](const pair<VBase *, int> & a, const pair<VBase *, int> & b){ return a.second > b.second; });

	return Candidates.front().first;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                          //
//                                  class Expand
//                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////


Expand::Expand()
{
}


Expand::~Expand()
{
}


string Expand::StateDescription() const
{
	if (!m_active) return "-";
	if (m_active) return "base #" + to_string(me().Bases().size());

	return "-";
}


bool Expand::ConditionOnUnits() const
{
	const int activeBases = count_if(me().Bases().begin(), me().Bases().end(), [](VBase * b){ return b->Active(); });

	if (him().IsProtoss())
	{
		if (activeBases == 1)
		{

		}
		else
		{
			if (me().Units(Terran_Vulture).size() < 3)
				return false;

			if (me().Units(Terran_Siege_Tank_Tank_Mode).size() < 1)
				return false;

			if (me().Army().KeepGoliathsAtHome())
				return false;

			if ((int)me().Units(Terran_Vulture).size() + (int)me().Units(Terran_Siege_Tank_Tank_Mode).size() + (int)me().Units(Terran_Goliath).size()/5 < 10)
				return false;

			if (me().Army().KeepTanksAtHome())
				if (me().Army().MyGroundUnitsAhead() < me().Army().HisGroundUnitsAhead() + 5)
					return false;
		}
/*
		if (activeBases == 1)
			if (ai()->GetStrategy()->Has<ShallowTwo>())
			{
				if (him().Buildings(Protoss_Photon_Cannon).size() >= 3)
				{
					if (me().Units(Terran_Siege_Tank_Tank_Mode).size() < 1)
						return false;
				}
				else if (him().MayDarkTemplar())
				{
					//if (me().Army().MyGroundUnitsAhead() < me().Army().HisGroundUnitsAhead() + 4)
					//	return false;
				}
				else
				{
					if (me().Army().MyGroundUnitsAhead() < me().Army().HisGroundUnitsAhead() + 16)
						return false;
				}

				if (him().MayReaver())
				{
					if (me().CompletedUnits(Terran_Wraith) < 1)
						return false;
				//	if (me().CompletedUnits(Terran_Siege_Tank_Tank_Mode) < 2)
				//		return false;
				}

			}
*/


	}
	else if (him().IsTerran())
	{
		int minVultures = 5;

		if (me().Bases().size() == 1)
		{
			minVultures = 13;

			if (him().Buildings(Terran_Bunker).size() >= 1) minVultures = 5;

			if (him().Buildings(Terran_Command_Center).size() >= 2) minVultures = 5;

			for (HisBuilding * b : him().Buildings(Terran_Command_Center))
				if (b->GetArea() != him().GetArea())
					minVultures = 5;

			if (ai()->GetStrategy()->Detected<WraithRush>()) minVultures = 3;
			
			if (ai()->GetStrategy()->Detected<GoliathRush>()) minVultures = 3;

			if (ai()->GetStrategy()->Detected<TerranFastExpand>()) minVultures = 3;
		}

		if ((int)me().Units(Terran_Vulture).size() < minVultures)
			return false;
	}
	else
	{
		int minVultures = 5;
/*
		if (me().Bases().size() == 1)
			if ((him().AllUnits(Zerg_Mutalisk).size() > 0) || (him().Buildings(Zerg_Spire).size() > 0))
				minVultures = 2;
*/
		if ((int)me().Units(Terran_Vulture).size() < minVultures)
			return false;

		if (me().Bases().size() >= 2)
			if (me().MineralsAvailable() < 500)
				if (him().HydraPressure_needVultures())
					if (me().Army().KeepTanksAtHome())
						return false;
	}

	return true;
}


bool Expand::ConditionOnActivity() const
{
	const int activeBases = count_if(me().Bases().begin(), me().Bases().end(), [](VBase * b){ return b->Active(); });

	if (activeBases >= 2)
	{
		if (me().MineralsAvailable() <= 600)
		{
			if (me().FactoryActivity() >= 9) return false;
			if (me().BarrackActivity() >= 9) return false;
		}
	}

	return true;
}


bool Expand::ConditionOnTech() const
{
	const int activeBases = count_if(me().Bases().begin(), me().Bases().end(), [](VBase * b){ return b->Active(); });

	const int mechProductionSites =
		me().Buildings(Terran_Factory).size() +
		me().Buildings(Terran_Starport).size();

//	const int bioProductionSites = me().Buildings(Terran_Barracks).size();

	if (him().IsProtoss())
		if (activeBases == 1)
		{
			//if (ai()->GetStrategy()->CounterGreedyProtoss())
//			if (ai()->GetStrategy()->CounterGreedyProtoss())
//				if (ai()->GetStrategy()->Active<Walling>())
//					return false;

			if (me().Buildings(Terran_Factory).empty())
				return false;

			if (me().MineralsAvailable() < 450)
				if (me().Buildings(Terran_Machine_Shop).size() < 1)
					return false;

	//		if (me().Buildings(Terran_Starport).size() == 0)
	//			return false;

		}

	if (activeBases >= 2)
		if (me().MineralsAvailable() <= 600)
		{
			if (!(me().Buildings(Terran_Missile_Turret).size() >= 8* me().Bases().size()))
			{
				if (mechProductionSites < min(12, 1*activeBases + 2)) return false;
			}
		}

	if (him().IsTerran())
	{
		if (ai()->GetStrategy()->Detected<GoliathRush>())
		{

		}
		else if (ai()->GetStrategy()->Detected<WraithRush>())
		{
//			return false;

			if (me().Buildings(Terran_Engineering_Bay).size() == 0)
				return false;

//			if (me().Buildings(Terran_Armory).size() == 0)
//				return false;
		}
		else if (ai()->GetStrategy()->Detected<TerranFastExpand>())
		{

		}
		else 
		{
			if (me().Buildings(Terran_Starport).size() == 0)
				return false;
		}
	}
	else
	{
		if (him().IsZerg())
			if (!ai()->GetStrategy()->Active<MassHydra>())
				if (!(
						(me().Buildings(Terran_Engineering_Bay).size() >= 1)
					))
					return false;

		if ((activeBases >= 3))
			if (!(
					(me().Buildings(Terran_Engineering_Bay).size() >= 1) &&
					(me().Units(Terran_Siege_Tank_Tank_Mode).size() >= 1)
				))
				return false;
	}
	return true;
}


bool Expand::Condition() const
{//return false;

	if (Interactive::expand) return true;

	if (auto s = ai()->GetStrategy()->Active<Stone>())
		if (s->CanExpand())
			return true;

	if (him().IsProtoss())
		if (me().ExceedingSCVsoldiers() < 5)
		{
			if (me().Bases().size() == 3)
				if (me().SupplyUsed() < 120)
					return false;

			if (me().Bases().size() == 4)
				if (me().SupplyUsed() < 150)
					return false;

			if (me().Bases().size() == 5)
				if (me().SupplyUsed() < 180)
					return false;
		}


	if (me().MineralsAvailable() > 800)
		if (me().SupplyUsed() > 150)
			return true;

	if (me().Bases().size() <= 2)
		if (me().StartingBase()->Minerals().size() < 6)
			if (!me().Army().KeepTanksAtHome())
			if (!me().Army().KeepGoliathsAtHome())
				return true;

	if (ai()->GetStrategy()->Detected<ZerglingRush>() ||
		ai()->GetStrategy()->Detected<ZealotRush>() ||
		ai()->GetStrategy()->Detected<DragoonRush>() ||
		ai()->GetStrategy()->Detected<CannonRush>() ||
		ai()->GetStrategy()->Detected<MarineRush>() ||
		ai()->GetStrategy()->Active<ProtossGreedy>() && !ai()->GetStrategy()->Active<ProtossGreedy>()->CanExpand() ||
		ai()->GetStrategy()->Active<MassHydra>() && !ai()->GetStrategy()->Active<MassHydra>()->CanExpand() ||
		ai()->GetStrategy()->Detected<GoliathRush>() && !ai()->GetStrategy()->Detected<GoliathRush>()->CanExpand() ||
		ai()->GetStrategy()->Active<BaseDefense>() && (me().Bases().size() <= 2) ||
		!him().AllUnits(Zerg_Lurker).empty() && (me().Bases().size() == 1) &&
			me().CompletedUnits(Terran_Siege_Tank_Tank_Mode) < 3 && me().TotalAvailableScans() == 0)
		return false;

	if (ConditionOnUnits())
//	if (ConditionOnActivity() || (me().MineralsAvailable() >= 350))
	if (me().Army().GoodValueInTime() ||
		(me().MineralsAvailable() >= 350) ||
		ai()->GetStrategy()->Detected<WraithRush>() && (me().Bases().size() == 1) ||
		ai()->GetStrategy()->Detected<TerranFastExpand>() && (me().Bases().size() == 1) ||
		me().Buildings(Terran_Missile_Turret).size() >= 8 * me().Bases().size() ||
		ai()->GetStrategy()->Active<MassHydra>() && ai()->GetStrategy()->Active<MassHydra>()->CanExpand() ||
		ai()->GetStrategy()->Detected<GoliathRush>() && ai()->GetStrategy()->Detected<GoliathRush>()->CanExpand() ||
		him().HydraPressure() &&
			(me().CompletedUnits(Terran_Siege_Tank_Tank_Mode) >= 1) &&
			(me().CompletedBuildings(Terran_Factory) >= 3))

//	if (ConditionOnTech() || (me().MineralsAvailable() >= 350))
	if (ConditionOnTech() || ((me().Bases().size() >= 3) && (me().MineralsAvailable() >= 1000)))
	{
		if (me().Bases().size() == 1)
		{
			if (him().HydraPressure())
				if (!(//me().Army().GroundLead() ||
						((me().CompletedUnits(Terran_Siege_Tank_Tank_Mode) >= 1) &&
						(me().CompletedBuildings(Terran_Factory) >= 3))))
					return false;

//			if (me().Army().GroundLead() || (me().MineralsAvailable() >= 350))
			//if (me().Army().GoodValueInTime() || (me().MineralsAvailable() >= 350))
			{
				//if (him().IsZerg() ||
				//	(me().Army().MyGroundUnitsAhead() >= me().Army().HisGroundUnitsAhead() + 3) ||
				//	(me().SupplyAvailable() >= 60) && (me().MineralsAvailable() >= 350) ||
				//	(me().SupplyAvailable() >= 70) ||
				//	(me().MineralsAvailable() >= 350))
					return true;
			}
		}
		else
		{
			if (ai()->Frame() - me().Bases().back()->CreationTime() > 1000)
				if (me().Army().GroundLead())
				{
				///	bw << "Expand::Condition 1" << endl;
					return true;
				}

			if (ai()->Frame() - me().Bases().back()->CreationTime() > 1000)
				if ((me().FactoryActivity() >= 9) && !ai()->GetStrategy()->Active<BaseDefense>())
				{
				///	bw << "Expand::Condition 2a" << endl;
					return true;
				}

			if (ai()->Frame() - me().Bases().back()->CreationTime() > 1000)
				if ((me().BarrackActivity() >= 9) && !ai()->GetStrategy()->Active<BaseDefense>())
				{
				///	bw << "Expand::Condition 2b" << endl;
					return true;
				}

			if (me().Bases().size() == 2)
				if (ai()->Frame() - me().Bases().back()->CreationTime() > 1000)
					if ((me().Army().Value() >= (him().IsProtoss() ? 2000 : 2500)) && !ai()->GetStrategy()->Active<BaseDefense>())
					{
					///	bw << "Expand::Condition 3" << endl;
						return true;
					}

			if (ai()->Frame() - me().Bases().back()->CreationTime() > 10000)
				if (Mining::Instances().size() < 20)
				{
				///	bw << "Expand::Condition 4" << endl;
					return true;
				}

			if (ai()->Frame() - me().Bases().back()->CreationTime() > 2000)
				if (me().MineralsAvailable() > 600)
				{
				///	bw << "Expand::Condition 5" << endl;
					return true;
				}

			if (ai()->Frame() - me().Bases().back()->CreationTime() > 1000)
				if (me().MineralsAvailable() > 600)
					if (me().SupplyUsed() > 150)
					{
					///	bw << "Expand::Condition 6" << endl;
						return true;
					}

			if (ai()->Frame() - me().Bases().back()->CreationTime() > 500)
				if (me().MineralsAvailable() > 1000)
				{
				///	bw << "Expand::Condition 7" << endl;
					return true;
				}

			if (me().MineralsAvailable() > 2000)
			{
			///	bw << "Expand::Condition 8" << endl;
				return true;
			}

			if (ai()->Frame() - me().Bases().back()->CreationTime() > 4000)
//				if (me().MineralsAvailable() > 100)
					if (me().Units(Terran_Goliath).size() >= 10)
						if (!me().Army().KeepGoliathsAtHome())
							if (me().Army().MyGroundUnitsAhead() > me().Army().HisGroundUnitsAhead() + 5)
								if (me().Units(Terran_Goliath).size() > me().Units(Terran_Siege_Tank_Tank_Mode).size())
								{
								///	bw << "Expand::Condition 9" << endl;
									return true;
								}

		}
	}

	return false;
}


VBase * Expand::FindNewBase() const
{
	VBase * newBase = nullptr;

	if (ai()->GetStrategy()->Active<Stone>())
		newBase = findNewBase();
	else if (me().Bases().size() == 1)
	{
		newBase = findNatural(me().StartingVBase());
	}
	else if (me().Bases().size() >= 2)
	{
		if (ai()->Frame() - me().Bases().back()->CreationTime() > 1000)
			newBase = findNewBase();
	}

	if (!newBase)
		for (VBase * base : me().Bases())
			if (base->Lost())
				if (base->ShouldRebuild())
				{
				///	bw << "ShouldRebuild " << base->BWEMPart()->Location() << " " << ai()->Frame() - base->CreationTime() << endl;
					if (none_of(him().Buildings().begin(), him().Buildings().end(), [base](const unique_ptr<HisBuilding> & b)
						{ return dist(b->TopLeft(), base->BWEMPart()->Location()) < 10; }))
					{
					///	bw << "ok" << endl;
						newBase = base;
						break;
					}
				}

	return newBase;
}


bool Expand::LiftCC() const
{
	if (ai()->GetStrategy()->Active<Stone>()) return false;
	if (him().IsProtoss() && (me().Bases().size() == 2))
	{
		if (me().Buildings(Terran_Command_Center).size() < 2) return true;
		if (me().Buildings(Terran_Command_Center)[1]->Pos().getDistance(findNatural(me().StartingVBase())->BWEMPart()->Center()) > 128) return true;
	}
	return false;
}


void Expand::OnFrame_v()
{
	if (m_active)
	{
		VBase * newBase = me().Bases().back();

		if (me().Bases().size() >= 5)
			if (ai()->Frame() - newBase->CreationTime() > 1200)
			{
			///	bw << "Expand: abort" << endl;
				newBase->SetLost();
				m_active = false;
				return;
			}

		if (newBase->Lost())
		{
			m_active = false;
			return;
		}

//		if (newBase->Active())
		if (newBase->Active(ai()->GetStrategy()->Active<Stone>() != nullptr || me().MineralsAvailable() > 2000))
		{
			vector<My<Terran_SCV> *> RemainingExploringSCVs;
			for (Exploring * pExplorer : Exploring::Instances())
				if (My<Terran_SCV> * pSCV = pExplorer->Agent()->IsMy<Terran_SCV>())
					if (pSCV->GetStronghold() == newBase->GetStronghold())
						RemainingExploringSCVs.push_back(pSCV);

			for (My<Terran_SCV> * pSCV : RemainingExploringSCVs)
				pSCV->ChangeBehavior<DefaultBehavior>(pSCV);

			m_active = false;
			return;
		}

		if (LiftCC())
		{

		}
		else
		{
			if (newBase->GetStronghold()->SCVs().size() < 3)
				if (none_of(Exploring::Instances().begin(), Exploring::Instances().end(), [newBase](const Exploring * e)
									{ return e->Agent()->Is(Terran_SCV) &&
											(e->Agent()->GetStronghold() == newBase->GetStronghold()); }))
				if (him().IsZerg() || ai()->GetStrategy()->Detected<WraithRush>() ||
					none_of(Constructing::Instances().begin(), Constructing::Instances().end(), [newBase](const Constructing * c)
									{ return c->Agent()->Is(Terran_SCV) &&
											(c->Agent()->GetStronghold() == newBase->GetStronghold()) &&
											(c->Type() == Terran_Command_Center); }))
					if (My<Terran_SCV> * pSCV = findFreeWorker(me().StartingVBase()))
					{
						pSCV->ChangeBehavior<Exploring>(pSCV, newBase->GetArea()->BWEMPart());
						pSCV->LeaveStronghold();
						pSCV->EnterStronghold(newBase->GetStronghold());
					}
		}
	}
	else
	{
		if (Condition())
		{
			if (VBase * newBase = FindNewBase())
			{
				if (newBase->Lost())
				{
					me().SetLastBase(newBase);
				}
				else
				{
					me().AddBase(newBase);
					assert_throw(me().Bases().size() >= 2);
					me().AddBaseStronghold(me().Bases().back());
				}

			///	bw << Name() << " started! " << endl; //ai()->SetDelay(100);
			///	if (newBase->Lost()) bw << "(recover lost base)" << endl;

				newBase->SetCreationTime();
				m_active = true;
				m_activeSince = ai()->Frame();
				return;
			}


		}
	}
}


} // namespace iron



