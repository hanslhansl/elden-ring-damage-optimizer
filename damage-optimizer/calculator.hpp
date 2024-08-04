#pragma once

#include "include.hpp"


namespace calculator
{
	template<class Key, class T>
	using map = std::map<Key, T>;
	using floating = double;
	struct Weapon;

	BETTER_ENUM(Attribute, int,
		STRENGTH,
		DEXTERITY,
		INTELLIGENCE,
		FAITH,
		ARCAINE
	);
	constexpr const char* attribute_to_json_string(Attribute at);
	constexpr auto attribute_json_string_table = better_enums::make_map(attribute_to_json_string);
	using Stats = std::array<int, Attribute::_size()>;
	using FullStats = std::array<int, 8>;
	Stats full_stats_to_stats(const FullStats& full_stats);

	BETTER_ENUM(Class, int,
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
	);
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

	BETTER_ENUM(AttackPowerType, int,
		PHYSICAL,
		MAGIC,
		FIRE,
		LIGHTNING,
		HOLY,
		POISON,
		SCARLET_ROT,
		BLEED,
		FROST,
		SLEEP,
		MADNESS,
		DEATH_BLIGHT
	);

	BETTER_ENUM(DamageType, int,
		PHYSICAL = AttackPowerType::PHYSICAL,
		MAGIC = AttackPowerType::MAGIC,
		FIRE = AttackPowerType::FIRE,
		LIGHTNING = AttackPowerType::LIGHTNING,
		HOLY = AttackPowerType::HOLY
	);
	BETTER_ENUM(StatusType, int,
		POISON = AttackPowerType::POISON,
		SCARLET_ROT = AttackPowerType::SCARLET_ROT,
		BLEED = AttackPowerType::BLEED,
		FROST = AttackPowerType::FROST,
		SLEEP = AttackPowerType::SLEEP,
		MADNESS = AttackPowerType::MADNESS,
		DEATH_BLIGHT = AttackPowerType::DEATH_BLIGHT
	);

	using ScalingCurve = std::array<floating, 149>;
	using AttributeScaling = std::array<floating, Attribute::_size()>;
	using AttackElementCorrects = std::array<AttributeScaling, AttackPowerType::_size()>;
	using AttackElementCorrectsById = map<int, AttackElementCorrects>;

	BETTER_ENUM(Affinity_, int,
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
		UNIQUE = -1
	);

	BETTER_ENUM(Type_, int,
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
		BEAST_CLAW = 95
	);

	constexpr auto ineffective_attribute_penalty = 0.4;
	constexpr auto defaultDamageCalcCorrectGraphId = 0;
	constexpr auto defaultStatusCalcCorrectGraphId = 6;

	struct AttackOptions
	{
		int available_stat_points;
		std::array<size_t, 2> upgrade_level;	// normal, somber
		bool two_handing;
		const static bool disable_two_handing_attack_power_bonus = false;
	};

	namespace attack_rating
	{
		namespace detail
		{
			struct base
			{
				static constexpr bool is_total_attack_power = false;
				static constexpr bool is_individual_attack_power = false;
				static constexpr bool is_individual_status_effect = false;
				static constexpr bool is_full = false;

				Stats stats{};
			};

			struct total_attack_power : base
			{
				static constexpr bool is_total_attack_power = true;

				floating total_attack_power;
				constexpr auto value() const { return total_attack_power; }
			};
			template<AttackPowerType I>
			struct individual_attack_power : base
			{
				static constexpr bool is_individual_attack_power = true;
				static constexpr AttackPowerType attack_power_type = I;

				floating individual_attack_power;
				constexpr auto value() const { return individual_attack_power; }
			};
			template<AttackPowerType I>
			struct individual_status_effect : base
			{
				static constexpr bool is_individual_status_effect = true;
				static constexpr AttackPowerType attack_power_type = I;

				floating individual_status_effect;
				constexpr auto value() const { return individual_status_effect; }
			};
			struct full : base
			{
				static constexpr bool is_full = true;

				const Weapon* weapon;
				std::array<size_t, 2> upgrade_level;
				bool two_handing;

				floating total_attack_power;
				std::array<std::pair<floating, std::array<floating, 2>>, DamageType::_size()> attack_power;
				std::array<std::pair<floating, std::array<floating, 2>>, StatusType::_size()> status_effect;
				std::array<floating, DamageType::_size()> spell_scaling;
				std::vector<AttackPowerType> ineffective_attack_power_types;
				std::vector<Attribute> ineffective_attributes;
			};

			template<typename B>
			struct sparse_attack_rating : B
			{

			};
		}

		using total = detail::sparse_attack_rating<detail::total_attack_power>;

		using physical = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::PHYSICAL>>;
		using magic = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::MAGIC>>;
		using fire = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::FIRE>>;
		using lightning = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::LIGHTNING>>;
		using holy = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::HOLY>>;

		using poison_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::POISON>>;
		using scarlet_rot_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::SCARLET_ROT>>;
		using bleed_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::BLEED>>;
		using frost_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::FROST>>;
		using sleep_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::SLEEP>>;
		using madness_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::MADNESS>>;
		using death_blight_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::DEATH_BLIGHT>>;

		using full = detail::sparse_attack_rating<detail::full>;
	}

	struct Weapon
	{
	private:
		inline static thread_local std::vector<Attribute> get_attack_rating_ineffective_attributes = []() { std::vector<Attribute> v{}; v.reserve(Attribute::_size()); return v; }();
	public:

		using Affinity = Affinity_;
		using Type = Type_;

		struct Filter
		{
			std::set<bool> dlc;
			std::set<Type> types;
			std::set<Affinity> affinities;
			std::set<std::string> base_names;

			bool operator()(const Weapon& weapon) const;
		};

		struct AllFilterOptions
		{
			std::vector<bool> dlc;
			//std::vector<bool> sorcery_tools;
			//std::vector<bool> incantation_tools;
			std::vector<Type> types;
			std::vector<Affinity> affinities;
			std::vector<std::string> base_names;
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
		const std::vector<std::array<floating, AttackPowerType::_size()>> base_attack_power;
		// map indicating which attack power types scale with which player attributes
		const AttackElementCorrectsById::mapped_type& attack_power_attribute_scaling;
		// map indicating which scaling curve is used for each attack power type
		const std::array<const ScalingCurve*, AttackPowerType::_size()> attack_power_scaling_curves;
		// thresholds and labels for each scaling grade (S, A, B, etc.) for this weapon. This isn't hardcoded for all weapons because it can be changed by mods.
		const std::array<std::pair<floating, std::string>, 6>& scaling_tiers;

		Stats adjust_stats_for_two_handing(bool two_handing, Stats stats) const;

		template<typename T>
		void get_attack_rating(const AttackOptions& attack_options_, const Stats& stats, T& result) const
		{
			auto adjusted_stats = this->adjust_stats_for_two_handing(attack_options_.two_handing, stats);

			result.stats = stats;
			if constexpr (T::is_total_attack_power)
				result.total_attack_power = 0.;
			if constexpr (T::is_full)
			{
				result.weapon = this;
				result.upgrade_level = attack_options_.upgrade_level;
				result.two_handing = attack_options_.two_handing;

				result.total_attack_power = 0.;
				result.ineffective_attack_power_types.reserve(AttackPowerType::_size());
			}

			for (auto attribute : Attribute::_values())
				if (adjusted_stats[attribute._to_index()] < this->requirements[attribute._to_index()])
					Weapon::get_attack_rating_ineffective_attributes.push_back(attribute);

			size_t upgrade_level;
			if (this->base_attack_power.size() == 1)
				upgrade_level = 0;
			else if (this->base_attack_power.size() == 11)
				upgrade_level = attack_options_.upgrade_level.at(1);
			else if (this->base_attack_power.size() == 26)
				upgrade_level = attack_options_.upgrade_level.at(0);
			else
				throw std::runtime_error("invalid base attack power size");

			bool is_sorcery_or_incantation_tool = this->sorcery_tool || this->incantation_tool;

			auto loop_cycle = [&](const AttackPowerType& attack_power_type)
				{
					auto temp_index = attack_power_type._to_index();
					auto base_attack_power = this->base_attack_power[upgrade_level][temp_index];

					if (base_attack_power != 0 || is_sorcery_or_incantation_tool)
					{
						auto is_damage_type = attack_power_type._to_index() <= AttackPowerType::HOLY;
						auto&& scaling_attributes = this->attack_power_attribute_scaling.at(attack_power_type._to_index());
						floating total_scaling = 1.;

						if (std::ranges::any_of(Weapon::get_attack_rating_ineffective_attributes,
							[&](Attribute ineffective_attribute) { return scaling_attributes[ineffective_attribute._to_index()] != 0; }))
						{
							total_scaling = 1. - ineffective_attribute_penalty;
							if constexpr (T::is_full)
								result.ineffective_attack_power_types.push_back(attack_power_type);
						}
						else
						{
							auto& effective_stats = (!attack_options_.disable_two_handing_attack_power_bonus && is_damage_type) ? adjusted_stats : stats;

							for (auto&& attribute : Attribute::_values())
							{
								auto&& attribute_correct = scaling_attributes.at(attribute._to_index());
								floating scaling{};

								if (attribute_correct != 0)
								{
									if (attribute_correct == 1)
										scaling = this->attribute_scaling.at(upgrade_level).at(attribute._to_index());
									else
										scaling = attribute_correct * this->attribute_scaling.at(upgrade_level).at(attribute._to_index()) / this->attribute_scaling.at(0).at(attribute._to_index());

									if (scaling != 0.)
										total_scaling += this->attack_power_scaling_curves[attack_power_type._to_index()]->operator[](effective_stats[attribute._to_index()]) * scaling;
								}
							}
						}

						if (base_attack_power != 0)
						{
							auto res = base_attack_power * total_scaling;

							if constexpr (T::is_total_attack_power)
								if (is_damage_type)
									result.total_attack_power += res;

							if constexpr (T::is_individual_attack_power)
								//if (attack_power_type == T::attack_power_type)
									result.individual_attack_power = res;

							if constexpr (T::is_individual_status_effect)
								//if (attack_power_type == T::attack_power_type)
									result.individual_status_effect = res;

							if constexpr (T::is_full)
							{
								if (is_damage_type)	// attack_power_type._to_index() <= AttackPowerType::HOLY
								{
									auto&& att_pwr = result.attack_power[attack_power_type._to_index()];
									att_pwr.first = res;
									att_pwr.second[0] = base_attack_power;
									att_pwr.second[1] = res - base_attack_power;
									result.total_attack_power += res;
								}
								else	// attack_power_type._to_index() > AttackPowerType::HOLY
								{
									auto&& att_pwr = result.status_effect[attack_power_type._to_index() - AttackPowerType::POISON];
									att_pwr.first = res;
									att_pwr.second[0] = base_attack_power;
									att_pwr.second[1] = res - base_attack_power;
								}
							}
						}

						if constexpr (T::is_full)
							if (is_damage_type && is_sorcery_or_incantation_tool)
								result.spell_scaling[attack_power_type._to_index()] = 100. * total_scaling;
					}
				};

			if constexpr (T::is_total_attack_power)
				for (auto&& attack_power_type : DamageType::_values())
					loop_cycle(AttackPowerType::_from_integral(attack_power_type));

			if constexpr (T::is_individual_attack_power)
				loop_cycle(T::attack_power_type);

			if constexpr (T::is_individual_status_effect)
				loop_cycle(T::attack_power_type);

			if constexpr (T::is_full)
				for (auto&& attack_power_type : AttackPowerType::_values())
					loop_cycle(attack_power_type);

			if constexpr (T::is_full)
			{
				result.ineffective_attributes = std::move(Weapon::get_attack_rating_ineffective_attributes);
				Weapon::get_attack_rating_ineffective_attributes.reserve(Attribute::_size());
			}
			Weapon::get_attack_rating_ineffective_attributes.clear();
		}
	};

	struct CalcCorrectGraphDict
	{
		int maxVal;
		floating maxGrowVal, adjPt;

		friend void from_json(const json& j, CalcCorrectGraphDict& c);
	};
	struct ReinforceTypesDict
	{
		AttributeScaling attack;	// index: AttackPowerType (if in ALL_DAMAGE_TYPES)
		AttributeScaling attributeScaling;	// index: Attribute
		std::array<int, 3> statusSpEffectId; // statusSpEffectId1, statusSpEffectId2, statusSpEffectId3

		friend void from_json(const json& j, ReinforceTypesDict& r);
	};

	constexpr size_t get_stat_variation_count(const int attribute_points, const Stats& min_stats)
	{
		constexpr auto N = Stats{}.size();
		constexpr auto UPPER = 99;
		const auto SUM = attribute_points;
		size_t count = 0;

		if (attribute_points > UPPER * min_stats.size())
			return 0;
			//throw std::invalid_argument("attribute_points must be <= " + std::to_string(UPPER) + " * " + std::to_string(N));

		if (std::ranges::any_of(min_stats, [](int v) { return v > UPPER; }))
			throw std::invalid_argument("min_stats must be <= " + std::to_string(UPPER));

		for (auto i = min_stats[0]; i <= std::min(UPPER, SUM); ++i)
		{
			auto SUM_i = SUM - i;
			for (auto j = min_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
			{
				auto SUM_i_j = SUM_i - j;

				if (0ll == min_stats[4])
				{
					auto a1 = std::max(min_stats[2], SUM_i_j - min_stats[3] - UPPER);
					auto b1 = std::min(UPPER, SUM_i_j - min_stats[3]);
					auto b1_a1_1 = b1 - a1 + 1;
					if (b1_a1_1 > 0)
						count += (1 - min_stats[3]) * b1_a1_1;

					auto a2 = a1;
					auto b2 = std::min(UPPER, SUM_i_j - UPPER - 1);
					auto b2_a2_1 = b2 - a2 + 1;
					if (b2_a2_1 > 0)
						count += UPPER * b2_a2_1;

					auto a3 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b3 = b1;
					auto b3_a3_1 = b3 - a3 + 1;
					if (b3_a3_1 > 0)
						count += SUM_i_j * b3_a3_1 - (a3 + b3) * b3_a3_1 / 2;

					auto a4 = std::max(min_stats[2], SUM_i_j - UPPER - UPPER);
					auto b4 = std::min(UPPER, SUM_i_j - min_stats[3] - UPPER - 1);
					auto b4_a4_1 = b4 - a4 + 1;
					if (b4_a4_1 > 0)
						count += (UPPER + 1 - SUM_i_j + UPPER) * b4_a4_1 + (a4 + b4) * b4_a4_1 / 2;

					auto a5 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b5 = b4;
					auto b5_a5_1 = b5 - a5 + 1;
					if (b5_a5_1 > 0)
						count += SUM_i_j * b5_a5_1;
				}
				else
				{
					auto a2 = std::max(min_stats[2], SUM_i_j - min_stats[3] - UPPER);
					auto b2 = std::min({ UPPER, SUM_i_j - UPPER - min_stats[4], SUM_i_j - UPPER - 1 });
					auto b2_a2_1 = b2 - a2 + 1;
					if (b2_a2_1 > 0)
						count += (1 + UPPER - min_stats[3]) * b2_a2_1;

					auto a3 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b3 = std::min(UPPER, SUM_i_j - UPPER - min_stats[4]);
					auto b3_a3_1 = b3 - a3 + 1;
					if (b3_a3_1 > 0)
						count += (1 + SUM_i_j - min_stats[3]) * b3_a3_1 - (a3 + b3) * b3_a3_1 / 1;

					auto a4 = std::max({ min_stats[2], SUM_i_j - min_stats[3] - UPPER, SUM_i_j - min_stats[4] - UPPER + 1 });
					auto b4 = std::min(UPPER, SUM_i_j - min_stats[4] - min_stats[3]);
					auto b4_a4_1 = b4 - a4 + 1;
					if (b4_a4_1 > 0)
						count += (SUM_i_j - min_stats[4] - min_stats[3] + 1) * b4_a4_1 - (a4 + b4) * b4_a4_1 / 2;

					auto a5 = std::max(min_stats[2], SUM_i_j - UPPER - UPPER);
					auto b5 = std::min({ UPPER, SUM_i_j - UPPER - 1 - min_stats[3], SUM_i_j - min_stats[4] - UPPER });
					auto b5_a5_1 = b5 - a5 + 1;
					if (b5_a5_1 > 0)
						count += (UPPER - SUM_i_j + UPPER + 1) * b5_a5_1 + (a5 + b5) * b5_a5_1 / 2;

					auto a7 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b7 = b5;
					auto b7_a7_1 = b7 - a7 + 1;
					if (b7_a7_1 > 0)
						count += (UPPER + 1) * b7_a7_1;

					auto a8 = std::max(min_stats[2], SUM_i_j - UPPER - min_stats[4] + 1);
					auto b8 = std::min(UPPER, SUM_i_j - min_stats[3] - UPPER - 1);
					auto b8_a8_1 = b8 - a8 + 1;
					if (b8_a8_1 > 0)
						count += (UPPER - min_stats[4] + 1) * b8_a8_1;
				}
			}
		}


		return count;
	}
	constexpr std::vector<Stats> get_stat_variations(const int attribute_points, const Stats& min_stats)
	{
		constexpr auto N = Stats{}.size();
		constexpr auto UPPER = 99;
		const auto SUM = attribute_points;

		auto possible_occurances = get_stat_variation_count(attribute_points, min_stats);
		if (possible_occurances == 0)
			return {};
		std::vector<Stats> stat_variations{};
		stat_variations.reserve(possible_occurances);

		for (auto i = min_stats[0]; i <= std::min(UPPER, SUM); ++i)
		{
			auto SUM_i = SUM - i;
			for (auto j = min_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
			{
				auto SUM_i_j = SUM_i - j;
				for (auto k = min_stats[2]; k <= std::min(UPPER, SUM_i_j); ++k)
				{
					auto SUM_i_j_k = SUM_i_j - k;
					for (auto l = min_stats[3]; l <= std::min(UPPER, SUM_i_j_k); ++l)
					{
						auto SUM_i_j_k_l = SUM_i_j_k - l;
						auto m = SUM_i_j_k_l;
						if (min_stats[4] <= m && m <= UPPER)
						{
							Stats indices = { i, j, k, l, m };
							stat_variations.push_back({ i, j, k, l, m });
						}
					}
				}
			}
		}

		return stat_variations;
	}

	struct OptimizationContext
	{
		using var_vec = std::variant<
			std::vector<attack_rating::total>,
			std::vector<attack_rating::physical>,
			std::vector<attack_rating::magic>,
			std::vector<attack_rating::fire>,
			std::vector<attack_rating::lightning>,
			std::vector<attack_rating::holy>,
			std::vector<attack_rating::poison_status>,
			std::vector<attack_rating::scarlet_rot_status>,
			std::vector<attack_rating::bleed_status>,
			std::vector<attack_rating::frost_status>,
			std::vector<attack_rating::sleep_status>,
			std::vector<attack_rating::madness_status>,
			std::vector<attack_rating::death_blight_status>
		>;

		var_vec optional_results;
		BS::thread_pool pool;
		const std::vector<const Weapon*>& weapons;
		AttackOptions atk_opt;

		template<typename T>
			requires requires (T t) { t.value(); }
		OptimizationContext(int threads, const std::vector<Stats>& stat_variations, const std::vector<const Weapon*>& filtered_weapons_, AttackOptions atk_opt_, std::type_identity<T>) :
			optional_results{}, pool(threads), weapons(filtered_weapons_), atk_opt(atk_opt_)
		{
			// create a vector of optional results for each weapon
			auto& optional_results = this->optional_results.emplace<std::vector<T>>();
			optional_results.resize(this->weapons.size());

			// process one weapon
			auto do_weapon = [&](size_t i)
				{
					const Weapon& weapon = *this->weapons[i];

					T intermediate_attack_rating{};
					T best_attack_rating{};

					// loop through all stat variations and find the one resulting in the best attack rating
					for (auto&& stats : stat_variations)
					{
						weapon.get_attack_rating(this->atk_opt, stats, intermediate_attack_rating);

						if (best_attack_rating.value() < intermediate_attack_rating.value())
							best_attack_rating = std::move(intermediate_attack_rating);
					}

					optional_results[i] = std::move(best_attack_rating);

					misc::print("completed " + std::to_string(i) + ": " + weapon.full_name + "\n");
				};

			//loop through all weapons and get the best attack rating each asynchronously
			this->pool.detach_sequence(0ull, this->weapons.size(), do_weapon);
		}

		attack_rating::full wait_and_get_result();
	};

	using FilteredWeapons = std::vector<const Weapon*>;

	class WeaponContainer
	{
		using WeaponDict = json;

		map<int, ScalingCurve> calcCorrectGraphsById{};
		AttackElementCorrectsById attackElementCorrectsById{};
		std::array<std::pair<floating, std::string>, 6> scalingTiers{};

		static ScalingCurve evaluate_CalcCorrectGraph(const std::vector<CalcCorrectGraphDict>& calcCorrectGraph);
	public:
		std::vector<Weapon> weapons{};

		WeaponContainer() = default;
		WeaponContainer(const std::filesystem::path& file_path);

		Weapon::AllFilterOptions get_all_filter_options() const;

		FilteredWeapons apply_filter(const Weapon::Filter& weapon_filter) const;
	};

	void test();
}

namespace nlohmann
{
	template<typename T>
	struct adl_serializer<calculator::map<calculator::AttackPowerType, T>>
	{
		static void from_json(const json& j, calculator::map<calculator::AttackPowerType, T>& m)
		{
			for (const auto& [apt_str, val] : j.items())
			{
				auto apt = calculator::AttackPowerType::_from_integral(std::stoi(apt_str));

				m[apt] = val.get<T>();
			}
		}
	};

	template<>
	struct adl_serializer<calculator::AttributeScaling>
	{
		static void from_json(const json& j, calculator::AttributeScaling& ac);
	};
	template<>
	struct adl_serializer<calculator::AttackElementCorrects>
	{
		static void from_json(const json& j, calculator::AttackElementCorrects& aec);
	};
	template<>
	struct adl_serializer<calculator::Stats>
	{
		static void from_json(const json& j, calculator::Stats& s);
	};
	template<>
	struct adl_serializer<calculator::Attribute>
	{
		static calculator::Attribute from_json(const json& j);
	};
	template<>
	struct adl_serializer<calculator::AttackPowerType>
	{
		static calculator::AttackPowerType from_json(const json& j);
	};
	template<>
	struct adl_serializer<calculator::Weapon::Affinity>
	{
		static calculator::Weapon::Affinity from_json(const json& j);
	};
	template<>
	struct adl_serializer<calculator::Weapon::Type>
	{
		static calculator::Weapon::Type from_json(const json& j);
	};
}

