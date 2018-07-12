#ifndef OPENING_H
#define OPENING_H

#include "macroitem.h"
#include "../units/expert.h"
#include <deque>
#include <unordered_map>

namespace iron
{
	class Opening
	{
	public:

		void AddItem(const MacroItem & item)		{ m_openingItems.push_back(item); }
		void PopItem()								{ m_openingItems.pop_front(); }

		MacroItem FirstItem() const					{ return *m_openingItems.begin(); }
		bool IsAllEmpty() const						{ return m_openingItems.empty() && m_keepProduceItems.empty(); }
		bool IsEmpty() const						{ return m_openingItems.empty(); }
		bool GetPreWalking() const					{ return m_preWalking; }
		void SetPreWalking(bool pw)					{ m_preWalking = pw; }
		bool GetBuildMachineShop() const			{ return m_getBuildMachineShop; }
		void SetBuildMachineShop()					{ m_getBuildMachineShop = true; }
		void UnsetBuildMachineShop()				{ m_getBuildMachineShop = false; }

		bool CheckRightExpert(Expert * e);
		bool CheckRightKeepExpert(Expert * e, bool checkSupply = false);
		void DrawTextOnScreen(int xx);
		void addKeepProduceItems(UnitType type, int amount);
		void finishOneItem(UnitType type);

	private:
		bool							m_preWalking = false;
		bool							m_getBuildMachineShop = false;
		deque<MacroItem>				m_openingItems;
		unordered_map<int, int>			m_keepProduceItems;
	};

	Opening GetOpeningByName(string name);
}

#endif