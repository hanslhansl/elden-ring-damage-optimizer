
#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <nlohmann/json.hpp>
#include <BS_thread_pool.hpp>

#include "hhh/misc.hpp"
#include "hhh/math/functions.hpp"


using namespace hhh;
//using json = nlohmann::ordered_json;
using json = nlohmann::json;


template<class Key, class T>
using map = std::map<Key, T>;
using floating = double;

enum class Attribute
{
	STRENGTH,
	DEXTERITY,
	INTELLIGENCE,
	FAITH,
	ARCAINE
};
constexpr std::array ALL_ATTRIBUTES = {
	Attribute::STRENGTH,
	Attribute::DEXTERITY,
	Attribute::INTELLIGENCE,
	Attribute::FAITH,
	Attribute::ARCAINE
};
std::array<std::string, ALL_ATTRIBUTES.size()> ALL_ATTRIBUTE_STRINGS = {
	"str",
	"dex",
	"int",
	"fai",
	"arc"
};
Attribute attribute_from_string(const std::string_view& str)
{
	for (int i = 0; i < ALL_ATTRIBUTES.size(); i++)
		if (str == ALL_ATTRIBUTE_STRINGS[i])
			return ALL_ATTRIBUTES[i];

	throw std::invalid_argument("invalid attribute string");
}
std::string attribute_to_string(const Attribute& attr)
{
	return ALL_ATTRIBUTE_STRINGS.at(int(attr));
}
using Stats = std::array<int, ALL_ATTRIBUTES.size()>;

enum class Class
{
	HERO,
	BANDIT,
	ASTROLOGER,
	WARRIOR,
	PRISONER,
	CONFESSOR,
	WRETCH,
	VAGABOND,
	PROPHET,
	SAMURAI
};
const map<Class, Stats> ALL_CLASS_STATS{
	{Class::HERO,		{ 16, 9,  7,  8, 11}},
	{Class::BANDIT,		{ 9, 13,  9,  8, 14}},
	{Class::ASTROLOGER,	{ 8, 12, 16,  7,  9}},
	{Class::WARRIOR,	{10, 16, 10,  8,  9}},
	{Class::PRISONER,	{11, 14, 14,  6,  9}},
	{Class::CONFESSOR,	{12, 12,  9, 14,  9}},
	{Class::WRETCH,		{10, 10, 10, 10, 10}},
	{Class::VAGABOND,	{14, 13,  9,  9,  7}},
	{Class::PROPHET,	{11, 10,  7, 16, 10}},
	{Class::SAMURAI,	{12, 15,  9,  8,  8}}
};

enum class AttackPowerType
{
	PHYSICAL = 0,
	MAGIC = 1,
	FIRE = 2,
	LIGHTNING = 3,
	HOLY = 4,
	POISON = 5,
	SCARLET_ROT = 6,
	BLEED = 7,
	FROST = 8,
	SLEEP = 9,
	MADNESS = 10,
	DEATH_BLIGHT = 11,
};
std::string attack_power_type_to_string(const AttackPowerType& apt)
{
	switch (apt)
	{
	case AttackPowerType::PHYSICAL:
		return "physical";
	case AttackPowerType::MAGIC:
		return "magic";
	case AttackPowerType::FIRE:
		return "fire";
	case AttackPowerType::LIGHTNING:
		return "lightning";
	case AttackPowerType::HOLY:
		return "holy";
	case AttackPowerType::POISON:
		return "poison";
	case AttackPowerType::SCARLET_ROT:
		return "scarlet_rot";
	case AttackPowerType::BLEED:
		return "bleed";
	case AttackPowerType::FROST:
		return "frost";
	case AttackPowerType::SLEEP:
		return "sleep";
	case AttackPowerType::MADNESS:
		return "madness";
	case AttackPowerType::DEATH_BLIGHT:
		return "death_blight";
	default:
		throw std::invalid_argument("invalid AttackPowerType");
	}
}
constexpr std::array ALL_ATTACK_POWER_TYPES = {
	AttackPowerType::PHYSICAL,
	AttackPowerType::MAGIC,
	AttackPowerType::FIRE,
	AttackPowerType::LIGHTNING,
	AttackPowerType::HOLY,
	AttackPowerType::POISON,
	AttackPowerType::SCARLET_ROT,
	AttackPowerType::BLEED,
	AttackPowerType::FROST,
	AttackPowerType::SLEEP,
	AttackPowerType::MADNESS,
	AttackPowerType::DEATH_BLIGHT
};
constexpr std::array ALL_DAMAGE_TYPES = {
	AttackPowerType::PHYSICAL,
	AttackPowerType::MAGIC,
	AttackPowerType::FIRE,
	AttackPowerType::LIGHTNING,
	AttackPowerType::HOLY
};
constexpr std::array ALL_STATUS_TYPES = {
	AttackPowerType::POISON,
	AttackPowerType::SCARLET_ROT,
	AttackPowerType::BLEED,
	AttackPowerType::FROST,
	AttackPowerType::SLEEP,
	AttackPowerType::MADNESS,
	AttackPowerType::DEATH_BLIGHT
};

using ScalingCurve = std::array<floating, 149>;
using AttributeScaling = std::array<floating, ALL_ATTRIBUTES.size()>;
using AttackElementCorrects = std::array<AttributeScaling, ALL_ATTACK_POWER_TYPES.size()>;
using AttackElementCorrectsById = map<int, AttackElementCorrects>;

void from_json(const json& j, Attribute& a)
{
	a = attribute_from_string(j.get<std::string>());
}
namespace nlohmann
{
	template<>
	struct adl_serializer<AttributeScaling>
	{
		static void from_json(const json& j, AttributeScaling& ac)
		{
			for (auto&& [attr_str, val] : j.items())
			{
				auto attr = attribute_from_string(attr_str);

				if (val.is_boolean())
					ac[int(attr)] = val.get<bool>();
				else if (val.is_number())
					ac[int(attr)] = val.get<floating>();
				else
					throw std::invalid_argument(std::string("invalid json type") + val.type_name());
			}
		}
	};
	template<>
	struct adl_serializer<AttackElementCorrects>
	{
		static void from_json(const json& j, AttackElementCorrects& aec)
		{
			for (auto&& [apt_str, ac_j] : j.items())
			{
				auto apt_int = std::stoi(apt_str);
				auto apt = AttackPowerType(apt_int);

				if (!std::ranges::contains(ALL_ATTACK_POWER_TYPES, apt))
					throw std::invalid_argument("invalid value for AttackPowerType");

				aec[apt_int] = ac_j.get<AttributeScaling>();
			}
		}
	};

	template<>
	struct adl_serializer<Stats>
	{
		static void from_json(const json& j, Stats& s)
		{
			for (int i = 0; i < ALL_ATTRIBUTES.size(); i++)
			{
				auto attr_str = attribute_to_string(ALL_ATTRIBUTES[i]);
				s[i] = j.value(attr_str, 0);
			}
		}
	};
	template<typename T>
	struct adl_serializer<map<AttackPowerType, T>>
	{
		static void from_json(const json& j, map<AttackPowerType, T>& m)
		{
			for (const auto& [apt_str, val] : j.items())
			{
				auto apt = AttackPowerType(std::stoi(apt_str));

				if (!std::ranges::contains(ALL_ATTACK_POWER_TYPES, apt))
					throw std::invalid_argument("invalid value for AttackPowerType");

				m[apt] = val.get<T>();
			}
		}
	};
}

constexpr auto ineffective_attribute_penalty = 0.4;
constexpr auto defaultDamageCalcCorrectGraphId = 0;
constexpr auto defaultStatusCalcCorrectGraphId = 6;

struct Weapon
{
	enum class Affinity
	{
		STANDARD = 0,
		HEAVY = 1,
		KEEN = 2,
		QUALITY = 3,
		MAGIC = 4,
		FIRE = 5,
		FLAME_ART = 6,
		LIGHTNING = 7,
		SACRED = 8,
		COLD = 9,
		POISON = 10,
		BLOOD = 11,
		OCCULT = 12,
		UNIQUE = -1,
	};

	enum class Type
	{
		DAGGER = 1,
		STRAIGHT_SWORD = 3,
		GREATSWORD = 5,
		COLOSSAL_SWORD = 7,
		CURVED_SWORD = 9,
		CURVED_GREATSWORD = 11,
		KATANA = 13,
		TWINBLADE = 14,
		THRUSTING_SWORD = 15,
		HEAVY_THRUSTING_SWORD = 16,
		AXE = 17,
		GREATAXE = 19,
		HAMMER = 21,
		GREAT_HAMMER = 23,
		FLAIL = 24,
		SPEAR = 25,
		GREAT_SPEAR = 28,
		HALBERD = 29,
		REAPER = 31,
		FIST = 35,
		CLAW = 37,
		WHIP = 39,
		COLOSSAL_WEAPON = 41,
		LIGHT_BOW = 50,
		BOW = 51,
		GREATBOW = 53,
		CROSSBOW = 55,
		BALLISTA = 56,
		GLINTSTONE_STAFF = 57,
		DUAL_CATALYST = 59,
		SACRED_SEAL = 61,
		SMALL_SHIELD = 65,
		MEDIUM_SHIELD = 67,
		GREATSHIELD = 69,
		TORCH = 87,
		HAND_TO_HAND = 88,
		PERFUME_BOTTLE = 89,
		THRUSTING_SHIELD = 90,
		THROWING_BLADE = 91,
		BACKHAND_BLADE = 92,
		LIGHT_GREATSWORD = 93,
		GREAT_KATANA = 94,
		BEAST_CLAW = 95,
	};

	struct AttackOptions
	{
		int player_level;
		Class starting_class;
		std::array<size_t, 2> upgrade_level;	// normal, somber
		bool two_handing;
		bool disable_two_handing_attack_power_bonus = false;
	};

	struct AttackRating
	{
		const Weapon* weapon;
		Stats stats;
		std::array<size_t, 2> upgrade_level;
		floating total_attack_power;
		std::array<std::pair<floating, std::array<floating, 2>>, ALL_ATTACK_POWER_TYPES.size()> attack_power;	// first = second[0] + second[1]
		std::array<floating, ALL_ATTACK_POWER_TYPES.size()> spell_scaling;
		std::vector<Attribute> ineffective_attributes;
		std::vector<AttackPowerType> ineffective_attack_power_types;

		void reset()
		{
			this->total_attack_power = 0.;
			this->attack_power.fill({});
			this->spell_scaling.fill({});
			this->ineffective_attributes.clear();
			this->ineffective_attributes.reserve(ALL_ATTRIBUTES.size());
			this->ineffective_attack_power_types.clear();
			this->ineffective_attack_power_types.reserve(ALL_ATTACK_POWER_TYPES.size());
		}
	};

	// the full unique name of the weapon, e.g. "Heavy Nightrider Glaive"
	const std::string full_name;
	// the base weapon name without an affinity specified, e.g. "Nightrider Glaive"
	const std::string base_name;
	// a wiki link for the weapon
	const std::string url;
	// true if the weapon was introduced with SOTE
	const bool dlc;
	// true if the weapon doesn't get a strength bonus when two-handing
	const bool paired;
	// true if this weapon can cast glintstone sorceries
	const bool sorcery_tool;
	// true if this weapon can cast incantations
	const bool incantation_tool;
	// the category of the weapon, e.g. Type.STRAIGHT_SWORD
	const Type type;
	// the affinity of the weapon, e.g. Affinity.HEAVY
	const Affinity affinity;
	// stat requirements necessary to use the weapon effectively (without an attack rating penalty)
	const Stats requirements;
	// scaling amount at each upgrade level (0-10 or 0-25) for each player attribute (e.g. Attribute.STRENGTH)
	const std::vector<AttributeScaling> attribute_scaling;
	// base attack power at each upgrade level for each attack power type
	const std::vector<std::array<floating, ALL_ATTACK_POWER_TYPES.size()>> base_attack_power;
	// map indicating which attack power types scale with which player attributes
	const AttackElementCorrectsById::mapped_type& attack_power_attribute_scaling;
	// map indicating which scaling curve is used for each attack power type
	const std::array<const ScalingCurve*, ALL_ATTACK_POWER_TYPES.size()> attack_power_scaling_curves;
	// thresholds and labels for each scaling grade (S, A, B, etc.) for this weapon. This isn't hardcoded for all weapons because it can be changed by mods.
	const std::array<std::pair<floating, std::string>, 6>& scaling_tiers;

	Stats adjust_stats_for_two_handing(bool two_handing, Stats stats) const
	{
		// Paired weapons do not get the two handing bonus
		if (this->paired)
			two_handing = false;

		// Bows and ballistae can only be two handed
		constexpr std::array bow_types = { Weapon::Type::LIGHT_BOW, Weapon::Type::BOW, Weapon::Type::GREATBOW, Weapon::Type::BALLISTA };
		if (std::ranges::contains(bow_types, this->type))
			two_handing = true;

		if (two_handing)
			stats.at(int(Attribute::STRENGTH)) = int(stats.at(int(Attribute::STRENGTH)) * 1.5);

		return stats;
	}

	void get_attack_rating(const AttackOptions& attack_options, const Stats& stats, AttackRating& result) const
	{
		auto adjusted_stats = this->adjust_stats_for_two_handing(attack_options.two_handing, stats);

		result.weapon = this;
		result.stats = stats;
		result.upgrade_level = attack_options.upgrade_level;

		for (auto attribute : ALL_ATTRIBUTES)
			if (adjusted_stats[int(attribute)] < this->requirements[int(attribute)])
				result.ineffective_attributes.push_back(attribute);

		size_t upgrade_level;
		if (this->base_attack_power.size() == 1)
			upgrade_level = 0;
		else if (this->base_attack_power.size() == 11)
			upgrade_level = attack_options.upgrade_level.at(1);
		else if (this->base_attack_power.size() == 26)
			upgrade_level = attack_options.upgrade_level.at(0);
		else
			throw std::runtime_error("invalid base attack power size");

		for (auto&& attack_power_type : ALL_ATTACK_POWER_TYPES)
		{
			auto base_attack_power = this->base_attack_power[upgrade_level][int(attack_power_type)];
			auto is_damage_type = attack_power_type <= AttackPowerType::HOLY;

			if (base_attack_power != 0 or this->sorcery_tool or this->incantation_tool)	// or (is_damage_type and (this->sorcery_tool or this->incantation_tool))
			{
				auto&& scaling_attributes = this->attack_power_attribute_scaling.at(int(attack_power_type));
				floating total_scaling = 1.;

				if (std::any_of(result.ineffective_attributes.begin(), result.ineffective_attributes.end(),
					[&](Attribute ineffective_attribute) { return scaling_attributes[int(ineffective_attribute)] != 0; }))
				{
					total_scaling = 1. - ineffective_attribute_penalty;
					result.ineffective_attack_power_types.push_back(attack_power_type);
				}
				else
				{
					auto& effective_stats = (!attack_options.disable_two_handing_attack_power_bonus && is_damage_type) ? adjusted_stats : stats;

					for (auto&& attribute : ALL_ATTRIBUTES)
					{
						auto&& attribute_correct = scaling_attributes.at(int(attribute));
						floating scaling{};

						if (attribute_correct != 0)
						{
							if (attribute_correct == 1)
								scaling = this->attribute_scaling.at(upgrade_level).at(int(attribute));
							else
								scaling = attribute_correct * this->attribute_scaling.at(upgrade_level).at(int(attribute)) / this->attribute_scaling.at(0).at(int(attribute));

							if (scaling != 0.)
								total_scaling += this->attack_power_scaling_curves[int(attack_power_type)]->operator[](effective_stats[int(attribute)]) * scaling;
						}
					}
				}

				if (base_attack_power != 0)
				{
					auto res = base_attack_power * total_scaling;
					auto&& att_pwr = result.attack_power.at(int(attack_power_type));
					att_pwr.first = res;
					att_pwr.second.at(0) = base_attack_power;
					att_pwr.second.at(1) = res - base_attack_power;
					result.total_attack_power += res;
				}

				if (is_damage_type and (this->sorcery_tool or this->incantation_tool))
					result.spell_scaling[int(attack_power_type)] = 100. * total_scaling;
			}
		}
	}
};

struct CalcCorrectGraphDict
{
	int maxVal;
	floating maxGrowVal, adjPt;

	friend void from_json(const json& j, CalcCorrectGraphDict& c)
	{
		j.at("maxVal").get_to(c.maxVal);
		j.at("maxGrowVal").get_to(c.maxGrowVal);
		j.at("adjPt").get_to(c.adjPt);
	}
};
struct ReinforceTypesDict
{
	AttributeScaling attack;	// index: AttackPowerType (if in ALL_DAMAGE_TYPES)
	AttributeScaling attributeScaling;	// index: Attribute
	std::array<int, 3> statusSpEffectId; // statusSpEffectId1, statusSpEffectId2, statusSpEffectId3

	friend void from_json(const json& j, ReinforceTypesDict& r)
	{
		auto&& attack_json = j.at("attack");
		for (auto dmg_type : ALL_DAMAGE_TYPES)
			r.attack.at(int(dmg_type)) = attack_json.value(std::to_string(int(dmg_type)), floating(0));

		r.attributeScaling = j.at("attributeScaling").get<AttributeScaling>();

		r.statusSpEffectId.at(0) = j.value("statusSpEffectId1", 0);
		r.statusSpEffectId.at(1) = j.value("statusSpEffectId2", 0);
		r.statusSpEffectId.at(2) = j.value("statusSpEffectId3", 0);
	}
};

class Main
{
	using WeaponDict = json;

	map<int, ScalingCurve> calcCorrectGraphsById{};
	AttackElementCorrectsById attackElementCorrectsById{};
	std::array<std::pair<floating, std::string>, 6> scalingTiers{};
public:
	std::vector<Weapon> weapons{};
private:

	static auto evaluate_CalcCorrectGraph(const std::vector<CalcCorrectGraphDict>& calcCorrectGraph)
	{
		std::array<floating, 149> arr{};

		for (size_t i = 1; i < calcCorrectGraph.size(); i++)
		{
			const CalcCorrectGraphDict& prevStage = calcCorrectGraph.at(i - 1);
			const CalcCorrectGraphDict& stage = calcCorrectGraph.at(i);

			auto minAttributeValue = i == 1 ? 1 : prevStage.maxVal + 1;
			auto maxAttributeValue = (i == calcCorrectGraph.size() - 1) ? 148 : stage.maxVal;

			auto attributeValue = minAttributeValue;
			while (attributeValue <= maxAttributeValue)
			{
				if (not arr.at(attributeValue))
				{
					auto ratio = floating(attributeValue - prevStage.maxVal) / floating(stage.maxVal - prevStage.maxVal);

					if (prevStage.adjPt > 0)
						ratio = std::pow(ratio, prevStage.adjPt);
					else if (prevStage.adjPt < 0)
						ratio = 1 - std::pow((1 - ratio), -prevStage.adjPt);

					arr[attributeValue] = prevStage.maxGrowVal + (stage.maxGrowVal - prevStage.maxGrowVal) * ratio;
				}
				attributeValue += 1;
			}
		}

		return arr;
	}

	constexpr auto get_all_stat_variations(const size_t attribute_points, const Stats LOWER) const
	{
		using T = int;

		constexpr T N = LOWER.size();
		constexpr T UPPER = 99;

		const T SUM = attribute_points;
		size_t possible_occurances = math::binomial_coefficient(size_t(SUM) + size_t(N) - 1, size_t(N) - 1);

		std::vector<Stats> all_stat_variations{};
		all_stat_variations.reserve(possible_occurances);

		for (T i = LOWER[0]; i <= std::min(UPPER, SUM); ++i)
		{
			T SUM_i = SUM - i;
			for (T j = LOWER[1]; j <= std::min(UPPER, SUM_i); ++j)
			{
				T SUM_i_j = SUM_i - j;
				for (T k = LOWER[2]; k <= std::min(UPPER, SUM_i_j); ++k)
				{
					T SUM_i_j_k = SUM_i_j - k;
					for (T l = LOWER[3]; l <= std::min(UPPER, SUM_i_j_k); ++l)
					{
						T SUM_i_j_k_l = SUM_i_j_k - l;
						T m = SUM_i_j_k_l;
						if (LOWER[4] <= m && m <= UPPER)
						{
							Stats indices = { i, j, k, l, m };
							all_stat_variations.push_back(indices);
						}
					}
				}
			}
		}

		return all_stat_variations;
	}

public:
	Main(const json& data)
	{
		for (auto&& [id_, calcCorrectGraph] : data.at("calcCorrectGraphs").items())
			this->calcCorrectGraphsById.emplace(std::stoi(id_), evaluate_CalcCorrectGraph(calcCorrectGraph.get<std::vector<CalcCorrectGraphDict>>()));

		for (auto&& [id, attackElementCorrect] : data.at("attackElementCorrects").items())
		{
			auto&& [inserted, success] = this->attackElementCorrectsById.emplace(std::stoi(id), attackElementCorrect);
			constexpr auto default_ = AttributeScaling{ false, false, false, false, true }; // default value
			inserted->second[int(AttackPowerType::POISON)] = default_;
			inserted->second[int(AttackPowerType::BLEED)] = default_;
			inserted->second[int(AttackPowerType::MADNESS)] = default_;
			inserted->second[int(AttackPowerType::SLEEP)] = default_;
		}

		map<int, std::vector<ReinforceTypesDict>> reinforceTypes{};
		for (auto&& [id, reinforceType] : data.at("reinforceTypes").items())
			reinforceTypes.emplace(std::stoi(id), reinforceType);

		map<int, map<AttackPowerType, int>> statusSpEffectParams{};
		for (auto&& [key, val] : data.at("statusSpEffectParams").items())
			statusSpEffectParams.try_emplace(std::stoi(key), val.get<map<AttackPowerType, int>>());

		this->scalingTiers = data.at("scalingTiers").get<decltype(this->scalingTiers)>();

		auto create_weapon = [&](const json& weapon_data) {
			auto&& attackElementCorrect = this->attackElementCorrectsById.at(weapon_data.at("attackElementCorrectId").get<int>());
			//auto attackElementCorrect = mdmap::items(this->attackElementCorrectsById, weapon_data.at("attackElementCorrectId").get<int>());

			const auto& reinforceParams = reinforceTypes.at(weapon_data.at("reinforceTypeId").get<int>());

			auto calcCorrectGraphIds = weapon_data.at("calcCorrectGraphIds").get<map<AttackPowerType, int>>();
			std::array<const ScalingCurve*, ALL_ATTACK_POWER_TYPES.size()> weaponCalcCorrectGraphs{};
			for (auto damage_type : ALL_DAMAGE_TYPES)
				weaponCalcCorrectGraphs.at(int(damage_type)) = &this->calcCorrectGraphsById.at(misc::map_get(calcCorrectGraphIds, damage_type, defaultDamageCalcCorrectGraphId));
			for (auto status_type : ALL_STATUS_TYPES)
				weaponCalcCorrectGraphs.at(int(status_type)) = &this->calcCorrectGraphsById.at(misc::map_get(calcCorrectGraphIds, status_type, defaultStatusCalcCorrectGraphId));

			auto unupgradedAttack = weapon_data.at("attack").get<std::vector<std::pair<AttackPowerType, int>>>();
			auto statusSpEffectParamIds = weapon_data.value("statusSpEffectParamIds", std::array<int, 3>{});
			std::vector<std::array<floating, ALL_ATTACK_POWER_TYPES.size()>> attack{};
			for (const auto& reinforceParam : reinforceParams)
			{
				auto& attack_at_upgrade_level = attack.emplace_back();
				for (const auto& [attackPowerType, unupgradedAttackPower] : unupgradedAttack)
					attack_at_upgrade_level.at(int(attackPowerType)) = unupgradedAttackPower * reinforceParam.attack.at(int(attackPowerType))/*default 0*/;

				int i = 0;
				for (const auto& spEffectParamId : statusSpEffectParamIds)
				{
					if (spEffectParamId)
					{
						const auto& statusSpEffectParam = statusSpEffectParams.at(spEffectParamId + reinforceParam.statusSpEffectId.at(i));
						for (const auto& [apt, val] : statusSpEffectParam)
							attack_at_upgrade_level.at(int(apt)) = val;
					}
					i++;
				}
			}

			auto unupgradedAttributeScaling = weapon_data.at("attributeScaling").get<std::vector<std::pair<Attribute, floating>>>();
			std::vector<AttributeScaling> attributeScaling{};
			for (const auto& reinforceParam : reinforceParams)
			{
				auto& foo = attributeScaling.emplace_back();
				for (const auto& [attribute, unupgradedScaling] : unupgradedAttributeScaling)
					foo.at(int(attribute)) = unupgradedScaling * reinforceParam.attributeScaling.at(int(attribute)/*default 0*/);
			}

			std::string weaponName = weapon_data.at("weaponName").get<std::string>();
			auto url_part = weaponName;
			std::ranges::replace(url_part, ' ', '_');

			return Weapon{
				weapon_data.at("name").get<std::string>(),
				std::move(weaponName),
				weapon_data.value("url", "https://eldenring.fandom.com/wiki/" + url_part),
				weapon_data.at("dlc").get<bool>(),
				weapon_data.value("paired", false),
				weapon_data.value("sorceryTool", false),
				weapon_data.value("incantationTool", false),
				weapon_data.at("weaponType").get<Weapon::Type>(),
				weapon_data.at("affinityId").get<Weapon::Affinity>(),
				weapon_data.at("requirements").get<Stats>(),
				std::move(attributeScaling),
				std::move(attack),
				std::move(attackElementCorrect),
				std::move(weaponCalcCorrectGraphs),
				this->scalingTiers };
			};

		const auto& weapons_data = data.at("weapons");
		this->weapons.reserve(weapons_data.size());
		for (auto&& weapon_data : weapons_data)
			this->weapons.emplace_back(create_weapon(weapon_data));

		misc::printl(this->weapons.size(), " weapons");
	}

	template<typename WeaponFilter, typename Proj>
		requires requires (const WeaponFilter& weapon_filter, const Weapon& w, const Proj& proj, const Weapon::AttackRating& war)
	{ { weapon_filter(w) } -> std::convertible_to<bool>; proj(war) < proj(war);
	}
	Weapon::AttackRating maximize_attack_rating(const WeaponFilter& weapon_filter, const Proj& proj, Weapon::AttackOptions attack_options) const
	{
		// get all possible stat variations
		const auto all_stat_variations = this->get_all_stat_variations(attack_options.player_level + 79, ALL_CLASS_STATS.at(attack_options.starting_class));

		// filter weapons
		std::vector<const Weapon*> filtered_weapons{};
		filtered_weapons.reserve(this->weapons.size());
		for (const auto& weapon : this->weapons)
			if (weapon_filter(weapon))
				filtered_weapons.push_back(&weapon);

		// print some stats
		auto total_combinations = filtered_weapons.size() * all_stat_variations.size();
		misc::printl(all_stat_variations.size(), " stat variations");
		misc::printl(filtered_weapons.size(), " weapons");
		misc::printl(total_combinations, " variations total");

		// process one weapon
		std::vector<std::optional<Weapon::AttackRating>> optional_results(filtered_weapons.size());
		auto do_weapon = [&](/*size_t ,const Weapon& weapon, std::optional<Weapon::AttackRating>& best_attack_rating, */size_t i)
			{
				const Weapon& weapon = *filtered_weapons[i];
				std::optional<Weapon::AttackRating>& best_attack_rating = optional_results[i];

				Weapon::AttackRating intermediate_attack_rating{};
				intermediate_attack_rating.ineffective_attack_power_types.reserve(ALL_ATTACK_POWER_TYPES.size());
				intermediate_attack_rating.ineffective_attributes.reserve(ALL_ATTRIBUTES.size());
				decltype(proj(intermediate_attack_rating)) best_attack_rating_proj_result{};

				// loop through all stat variations and find the one resulting in the best attack rating
				for (auto&& stats : all_stat_variations)
				{
					weapon.get_attack_rating(attack_options, stats, intermediate_attack_rating);
					auto intermediate_attack_rating_proj_result = proj(intermediate_attack_rating);

					if (!best_attack_rating.has_value() or best_attack_rating_proj_result < intermediate_attack_rating_proj_result)
					{
						best_attack_rating_proj_result = std::move(intermediate_attack_rating_proj_result);
						best_attack_rating.emplace(std::move(intermediate_attack_rating));
					}

					intermediate_attack_rating.reset();

					//break;
				}
				misc::print("completed " + std::to_string(i) + ": " + weapon.full_name + "\n");
			};


		// create a thread pool
		BS::thread_pool pool(20);

		// loop through all weapons and get the best attack rating each
		pool.detach_sequence(0ull, filtered_weapons.size(), do_weapon);

		// wait for all threads to finish
		pool.wait();

		// loop through all optional attack ratings and get the best one
		std::optional<Weapon::AttackRating> result{};
		for (auto&& optional_result : optional_results)
			if (optional_result.has_value() and (!result.has_value() or proj(result.value()) < proj(optional_result.value())))
				result.emplace(std::move(optional_result.value()));

		return result.value();
	}
};



int main(int argc, char* argv[])
{
	std::string file = "D:\\Paul\\Computer\\Programmieren\\C++\\Haupt-Projektmappe\\elden_ring_damage_calculator\\regulation-vanilla-v1.12.3.js";
	std::ifstream stream(file);
	json data = json::parse(stream);

	auto&& [main, time1] = misc::TimeFunctionExecution([&]() { return *new Main(data); });
	misc::printl();

	Weapon::AttackOptions atk_options = { 161, Class::VAGABOND, { 24, 10 }, true, false };
	auto [attack_rating, time2] = misc::TimeFunctionExecution([&]() {
		return main.maximize_attack_rating([](const Weapon& w) { return w.dlc; }, [](const Weapon::AttackRating& war) { return war.total_attack_power; }, atk_options); });
	misc::printl();

	misc::print("stats: ");
	for (auto stat : attack_rating.stats)
		misc::print(int(stat), " ");
	misc::printl("\n");

	misc::printl(attack_rating.weapon->full_name, ": ", attack_rating.total_attack_power);
	misc::printl();
	for (int i = 0; i < attack_rating.attack_power.size(); i++)
		if (attack_rating.attack_power.at(i).first != 0)
			misc::printl(attack_power_type_to_string(AttackPowerType(i)), ": ", attack_rating.attack_power.at(i).second.at(0), " + ", attack_rating.attack_power.at(i).second.at(1),
				" = ", attack_rating.attack_power.at(i).first);
	misc::printl();

	/*for (auto&& [apt, att] : attack_rating.split_attack_power)
		misc::printl(attack_power_type_to_string(apt), ": ", att[0], " + ", att[1]);
	misc::printl();*/

	misc::printl("create weapon list: ", time1);
	misc::printl("query best stats: ", time2);



	return 0;
}