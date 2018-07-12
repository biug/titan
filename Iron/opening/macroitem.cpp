#include "macroitem.h"

#include <regex>

namespace iron
{
	int GetIntFromString(const string & s)
	{
		stringstream ss(s);
		int a = 0;
		ss >> a;
		return a;
	}

	// Make a Type string look pretty for the UI
	string NiceTypeName(const string & s)
	{
		string inputName(s);
		replace(inputName.begin(), inputName.end(), '_', ' ');
		transform(inputName.begin(), inputName.end(), inputName.begin(), ::tolower);
		return inputName;
	}

	MacroLocation MacroItem::GetMacroLocationFromString(string & s)
	{
		if (s == "natural")
		{
			return MacroLocation::Natural;
		}
		if (s == "main")
		{
			return MacroLocation::Main;
		}

		return MacroLocation::Anywhere;
	}

	MacroItem::MacroItem()
		: m_type(MacroItems::Default)
		, m_race(Races::None)
		, m_macroLocation(MacroLocation::Anywhere)
	{
	}

	// Create a MacroItem from its name, like "drone" or "hatchery @ minonly".
	// String comparison here is case-insensitive.
	MacroItem::MacroItem(const string & name)
		: m_type(MacroItems::Default)
		, m_race(Races::None)
		, m_macroLocation(MacroLocation::Anywhere)
	{
		string inputName(NiceTypeName(name));

		// Commands like "go gas until 100". 100 is the amount.
		if (inputName.substr(0, 3) == string("go "))
		{
			for (const MacroCommandType t : MacroCommand::AllCommandTypes())
			{
				string commandName = MacroCommand::GetName(t);
				if (MacroCommand::HasArgument(t))
				{
					// There's an argument. Match the command name and parse out the argument.
					regex commandWithArgRegex(commandName + " (\\d+)");
					smatch m;
					if (regex_match(inputName, m, commandWithArgRegex)) {
						int amount = GetIntFromString(m[1].str());
						if (amount >= 0) {
							*this = MacroItem(t, amount);
							return;
						}
					}
				}
				else
				{
					// No argument. Just compare for equality.
					if (commandName == inputName)
					{
						*this = MacroItem(t);
						return;
					}
				}
			}
		}

		MacroLocation specifiedMacroLocation(MacroLocation::Anywhere);    // the default

																		  // Buildings can specify a location, like "hatchery @ expo".
																		  // It's meaningless and ignored for anything except a building.
																		  // Here we parse out the building and its location.
																		  // Since buildings are units, only UnitType below sets _macroLocation.
		regex macroLocationRegex("([a-z_ ]+[a-z])\\s*\\@\\s*([a-z][a-z ]+)");
		smatch m;
		if (regex_match(inputName, m, macroLocationRegex)) {
			specifiedMacroLocation = GetMacroLocationFromString(m[2].str());
			// Don't change inputName before using the results from the regex.
			// Fix via gnuborg, who credited it to jaj22.
			inputName = m[1].str();
		}
		for (const UnitType & unitType : UnitTypes::allUnitTypes())
		{
			// check to see if the names match exactly
			string typeName(NiceTypeName(unitType.getName()));
			if (typeName == inputName)
			{
				*this = MacroItem(UnitType(unitType));
				m_macroLocation = specifiedMacroLocation;
				return;
			}

			// check to see if the names match without the race prefix
			string raceName = unitType.getRace().getName();
			transform(raceName.begin(), raceName.end(), raceName.begin(), ::tolower);
			if ((typeName.length() > raceName.length()) && (typeName.compare(raceName.length() + 1, typeName.length(), inputName) == 0))
			{
				*this = MacroItem(UnitType(unitType));
				m_macroLocation = specifiedMacroLocation;
				return;
			}
		}

		for (const TechType & techType : TechTypes::allTechTypes())
		{
			string typeName(NiceTypeName(techType.getName()));
			if (typeName == inputName)
			{
				*this = MacroItem(techType);
				return;
			}
		}

		for (const UpgradeType & upgradeType : UpgradeTypes::allUpgradeTypes())
		{
			string typeName(NiceTypeName(upgradeType.getName()));
			if (typeName == inputName)
			{
				*this = MacroItem(upgradeType);
				return;
			}
		}

		m_name = GetName();
	}

	MacroItem::MacroItem(UnitType t)
		: m_unitType(t)
		, m_type(MacroItems::Unit)
		, m_race(t.getRace())
		, m_macroLocation(MacroLocation::Anywhere)
	{
		m_name = GetName();
	}

	MacroItem::MacroItem(UnitType t, MacroLocation loc)
		: m_unitType(t)
		, m_type(MacroItems::Unit)
		, m_race(t.getRace())
		, m_macroLocation(loc)
	{
		m_name = GetName();
	}

	MacroItem::MacroItem(TechType t)
		: m_techType(t)
		, m_type(MacroItems::Tech)
		, m_race(t.getRace())
		, m_macroLocation(MacroLocation::Anywhere)
	{
		m_name = GetName();
	}

	MacroItem::MacroItem(UpgradeType t)
		: m_upgradeType(t)
		, m_type(MacroItems::Upgrade)
		, m_race(t.getRace())
		, m_macroLocation(MacroLocation::Anywhere)
	{
		m_name = GetName();
	}

	MacroItem::MacroItem(MacroCommandType t)
		: m_macroCommandType(t)
		, m_type(MacroItems::Command)
		, m_race(Races::Terran)
		, m_macroLocation(MacroLocation::Anywhere)
	{
		m_name = GetName();
	}

	MacroItem::MacroItem(MacroCommandType t, int amount)
		: m_macroCommandType(t, amount)
		, m_type(MacroItems::Command)
		, m_race(Races::Terran)     // irrelevant
		, m_macroLocation(MacroLocation::Anywhere)
	{
		m_name = GetName();
	}

	const size_t & MacroItem::type() const
	{
		return m_type;
	}

	bool MacroItem::IsUnit() const
	{
		return m_type == MacroItems::Unit;
	}

	bool MacroItem::IsTech() const
	{
		return m_type == MacroItems::Tech;
	}

	bool MacroItem::IsUpgrade() const
	{
		return m_type == MacroItems::Upgrade;
	}

	bool MacroItem::IsCommand() const
	{
		return m_type == MacroItems::Command;
	}

	const Race & MacroItem::GetRace() const
	{
		return m_race;
	}

	bool MacroItem::IsBuilding()	const
	{
		return m_type == MacroItems::Unit && m_unitType.isBuilding();
	}

	bool MacroItem::IsBuildingAddon() const
	{
		return m_type == MacroItems::Unit && m_unitType.isAddon();
	}

	bool MacroItem::IsRefinery()	const
	{
		return m_type == MacroItems::Unit && m_unitType.isRefinery();
	}

	// The standard supply unit, ignoring the hatchery (which provides 1 supply).
	bool MacroItem::IsSupply() const
	{
		return IsUnit() &&
			(m_unitType == Terran_Supply_Depot
				|| m_unitType == Protoss_Pylon
				|| m_unitType == Zerg_Overlord);
	}

	const UnitType & MacroItem::GetUnitType() const
	{
		return m_unitType;
	}

	const TechType & MacroItem::GetTechType() const
	{
		return m_techType;
	}

	const UpgradeType & MacroItem::GetUpgradeType() const
	{
		return m_upgradeType;
	}

	const MacroCommand MacroItem::GetCommandType() const
	{
		return m_macroCommandType;
	}

	const MacroLocation MacroItem::GetMacroLocation() const
	{
		return m_macroLocation;
	}

	int MacroItem::supplyRequired() const
	{
		if (IsUnit())
		{
			return m_unitType.supplyRequired();
		}
		return 0;
	}

	// NOTE Because upgrades vary in price with level, this is context dependent.
	int MacroItem::mineralPrice() const
	{
		if (IsCommand())
		{
			return 0;
		}
		if (IsUnit())
		{
			return m_unitType.mineralPrice();
		}
		if (IsTech())
		{
			return m_techType.mineralPrice();
		}
		if (IsUpgrade())
		{
			if (m_upgradeType.maxRepeats() > 1 && Broodwar->self()->getUpgradeLevel(m_upgradeType) > 0)
			{
				return m_upgradeType.mineralPrice(1 + Broodwar->self()->getUpgradeLevel(m_upgradeType));
			}
			return m_upgradeType.mineralPrice();
		}

		return 0;
	}

	// NOTE Because upgrades vary in price with level, this is context dependent.
	int MacroItem::gasPrice() const
	{
		if (IsCommand()) {
			return 0;
		}
		if (IsUnit())
		{
			return m_unitType.gasPrice();
		}
		if (IsTech())
		{
			return m_techType.gasPrice();
		}
		if (IsUpgrade())
		{
			if (m_upgradeType.maxRepeats() > 1 && Broodwar->self()->getUpgradeLevel(m_upgradeType) > 0)
			{
				return m_upgradeType.gasPrice(1 + Broodwar->self()->getUpgradeLevel(m_upgradeType));
			}
			return m_upgradeType.gasPrice();
		}

		return 0;
	}

	UnitType MacroItem::whatBuilds() const
	{
		if (IsCommand()) {
			return UnitTypes::None;
		}
		UnitType buildType = IsUnit() ? m_unitType.whatBuilds().first : (IsTech() ? m_techType.whatResearches() : m_upgradeType.whatUpgrades());
		return buildType;
	}

	string MacroItem::GetName() const
	{
		if (IsUnit())
		{
			return m_unitType.getName();
		}
		if (IsUpgrade())
		{
			return m_upgradeType.getName();
		}
		if (IsTech())
		{
			return m_techType.getName();
		}
		if (IsCommand())
		{
			return m_macroCommandType.GetName();
		}

		return "error";
	}

}