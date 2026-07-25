# Kachel — Face Redesign Research (Design Council, 2026-07-25)

Five-lens research council + synthesis, commissioned by Josch's redesign brief:
weather must be legible, clock can shrink, user-centered daypart thinking,
Little Signals / Nothing inspiration, "more provocative in its calmness."
Direction decision pending (Josch + agent). Constraints of SPEC §5 and the
hue law (docs/DESIGN_FACE.md §G) were treated as hard throughout.

# Research digest

1. **(user-context)** Wrists, phones, and the oven already own HH:MM — 50% of watch use is pure timekeeping, 72% sub-5s glances; the tile's differentiated value is context, not redundant time. The 176 px clock is spending the whole face on the one datum nobody needs from it.
2. **(user-context)** The primary datum should rotate by daypart: rush = weather-as-decision (bike/transit/layers), cooking = timer, evening = tomorrow's first event, day = clock-as-scenery, deep night = nothing.
3. **(encoding)** v1's weather fails structurally, not by degree: it's a modifier on a moving time-of-day baseline, forcing conjunction search. Fix = absolute encoding, fixed position, 3–4 discriminable levels, exclusive channel ownership.
4. **(encoding)** Channels still free under the hue law: position, discrete count (pre-attentive to ~4), fill-level, horizon-edge shape, dither texture/grain, and *stepped* lightness within owned hues. Motion and saturation are closed.
5. **(calm-canon)** The strongest canon move is indirect display (Little Signals Air, Ishii's rain): weather should be rendered as *behavior of the field* — grain, veil, edge diffusion — not tint. Static texture is legal where animation is not.
6. **(calm-canon)** Periphery→center is user-pulled: tap promotes a datum to center for seconds, then it self-decays (Button's twist-for-detail; mui's disappearing display).
7. **(typography)** NDot is legally unusable — proprietary, Nothing-exclusive, no legitimate clone. Doto (OFL, square-leaning dots, integer dot pitch) + Departure Mono 22 px (2× its native grid) is the license-safe Nothing voice; square dots also kill the Bayer-dither moire risk.
8. **(typography)** Nothing's transferable discipline is rationing: dot face for one hero datum only; everything secondary in a quiet face. Flash budget: three font instances total.
9. **(precedents)** The clichés Josch is reacting to: giant centered grotesk clock, corner weather icon+temp, widget grids, content churn. Every SPEC §5 rule inverts a documented industry failure.
10. **(precedents)** Steal-worthy: Nest's state collapse (the darkest state is the most designed), StandBy's *total* night tint, horological sector zoning (position = identity, most zones empty), and analog shape-reading (time as pre-attentive angle) — which no shipped display does.
11. **(contradiction — glyphs)** Calm-canon and precedents say icons are "display" and the most-copied cliché; encoding cites Matthews et al. that symbolic marks beat pure-ambient encodings on learnability. Council resolution: one hairline, fixed-anchor, bone-colored mark ≤22 px whose *absence* is meaningful is calm-legal; icon rows and condition glyph-grammars are not.
12. **(contradiction — clock size)** Calm-canon wants a near-vanished 64 px tap-to-bloom clock; user-context keeps the clock primary in Day/Evening dayparts; typography lands at 120 px. Unresolved by evidence — this is a taste decision for Josch, and the concepts below deliberately span the range.

---

# Direction concepts

## 1. OPEN SKY — pure field, the display behaves like weather

**One-line thesis:** The tile stops describing the sky and starts *being* one — no glyph, no numeral for weather, just a field whose material behavior (veil, grain, diffusion) you learn to read like a window; the provocation is that it dares to look like nothing.

**Info architecture:** Field is the permanent primary. Rush: weather amplitude is boosted (field is the datum). Day/Evening: field relaxes to scenery, small clock is primary. 22:00–00:00: amber clock on black; 00:00–06:00: black, time on tap. Timer guest card preempts bottom band; calendar line appears only when next event <2 h. Cut: everything else — temp, agenda, Bring, all numbers.
**Weather treatment:** Splits gradient semantics to restore channel exclusivity (encoding proposal A + calm-canon A). Top 160 px = weather-owned slate band: cloud cover in **3 absolute lightness steps** (clear = near-black, partly = L0.25, overcast = L0.40 flat); precip-next-12h = veil descending toward horizon, **fill-level ∝ mm, 3 steps**; fog = horizon edge diffuses 40 px; active rain = vertical anisotropy in the dither grain; snow = brightened granular ground band. Legible where v1 wasn't because the encoding is *absolute at a fixed zone with few levels*, not a 1-JND nudge on a moving baseline.
**Clock treatment:** Recedes to ~100 px Doto at a fixed lower-center anchor. Tap anywhere: time blooms to center for 5 s in one 500 ms ease, then recedes (mui/Weiser).
**Typography:** Doto (wght ~320, ROND 25) 100 px, digits+colon subset; Departure Mono 22 px for guest lines. 2 font instances.
**Field/gradient behavior:** Bottom two stops = time-of-day phase (unchanged, its legible job); sky band = weather; dust-violet H330 shift preserved for air.
**Motion:** Horizon breathing 6 cpm unchanged; every weather state change one 800 ms ease then static; grain is static texture, never animated.
**Spec deltas:** Texture/grain as a channel is new (not in the pre-attentive four — needs Josch approval + hardware prototype for banding); clock size/typeface replaces Inter Tight 176 (DESIGN_FACE rewrite); tap-to-bloom is a new interaction; gradient semantic split amends the 3-stop model.
**Weiser test:** Field — informs from periphery ✓. Clock — periphery gives rough time, tap rewards attention ✓. Guest cards — unchanged ✓. Nothing else exists.
**Risk:** Rain vs. snow stays ambiguous; temperature is absent ("do I need a jacket?" goes unanswered); a genuine one-week learning curve; grain may read as a rendering defect at 2–3 m.

## 2. THREE STRATA — the daypart engine (hybrid)

**One-line thesis:** The face answers the exact check you were about to pull a phone out for — and does it by rotating *which* fixed stratum is loud, so it carries strictly more information while showing less at any given moment.

**Info architecture:** Three permanent strata; relevance moves through intensity/fill, never position. **FIELD** (atmosphere: phase + weather + air), **MARK** (clock, position eternal, size/brightness steps per daypart), **SLOT** (one amber bottom band = "the next thing that needs you": leave-by event → timer preempts → tomorrow's first event → empty; empty slot *is* the Yohaku signal). Rush: weather tri-state primary (dry-bike / rain-transit / cold-layer via field amplitude + precip marks), clock demoted. Day: clock primary, slot empty. Cooking (timer OR PM2.5 spike): timer primary; violet escalation suppressed, escalates only if the spike persists ~30 min. Evening: tomorrow's first event primary. 22–00: small amber clock, total night tint (StandBy rule — no element escapes). 00–06: black. Cut: raw sensor values, filter %, Bring, agendas, date, forecasts.
**Weather treatment:** Encoding B. Field carries mood via Open Sky's absolute sky-band steps; the *decision* lives in **1–3 hairline droplet strokes, 2 px, bone L0.80, at fixed anchor (56, 388)** — present only when precip >0.5 mm/12 h, count = intensity tercile, frozen precip = strokes rotated to ticks. Absence = dry. Legible because presence/count of ≤3 marks at a known corner is pre-attentive numerosity, day one.
**Clock treatment:** Doto 120 px, anchor (240, 200) kept forever; brightness/weight steps per daypart via one 300–800 ms ease.
**Typography:** Doto 120 (clock), Doto 44 (timer countdown, amber, same dot voice), Departure Mono 22 (slot text, umlauts covered). 3 instances — exactly the flash budget.
**Field/gradient behavior:** Phase + weather sky band + air hue; weather amplitude graduates from modifier to subject during rush, then relaxes — same channel, stepped legibility.
**Motion:** Daypart transitions are single eases; breathing unchanged; droplet marks ease in once (500 ms) then static.
**Spec deltas:** "One primary datum per state" is honored but *state* now means daypart — DESIGN_FACE needs a daypart-state table; droplet marks bend "abstract before display" (flag for Josch, council argues absence-is-meaningful keeps it calm); clock typeface/size swap; cooking-mode violet suppression is a new escalation rule.
**Weiser test:** Field ✓ (periphery). Clock ✓. Slot — informs peripherally (something needs me / nothing does), tap for event text ✓. Droplet marks — periphery: presence/count; attention: bike-or-tram decision ✓.
**Risk:** Most moving parts of any concept; the tri-state weather derivation can be *wrong* (a bad "dry-bike" call at 8 a.m. costs trust faster than any aesthetic win); daypart engine needs careful hysteresis at boundaries.

## 3. COMPLICATION — the day as a dial

**One-line thesis:** Time becomes shape, not digits — a horizon-to-horizon day arc with the sun's position and your next events as geometry, read pre-attentively the way a watch hand is; no shipped smart display has ever done this.

**Info architecture:** One low arc spanning the frame = today, sunrise to sunset. A marker travels it (sun position = time-of-day + daylight remaining as a single form). Up to 3 event pegs sit at their positions ahead of the marker, filling as they near, amber inside one hour; tap a peg = event text for 8 s, then decay. Arc empty behind the marker; empty arc ahead = free day (Yohaku). Small clock above as the *complication*, not the dial. Night: arc gone, amber clock only, then black. Cut: everything numeric except HH:MM.
**Weather treatment:** The sky above the arc is weather-owned, borrowing Open Sky's absolute steps: cloud lightness (3 steps), precip veil fill, fog diffusion of the arc line itself. The arc gives it a stable reference frame — the missing invariant anchor the Orb had and v1 lacked.
**Clock treatment:** Doto 80 px, fixed above the arc's apex; ranked second to shape, horology-style.
**Typography:** Doto 80 (clock), Departure Mono 22 (tapped peg detail). 2 instances.
**Field/gradient behavior:** Above arc = weather sky; below arc = ground/phase warmth; air violet shifts the whole field as today.
**Motion:** Sun marker advances ~1 px per 2 min — below perception, technically continuous (flag); peg fills are single eases; breathing kept.
**Spec deltas:** Shape/angle as a channel is outside the sanctioned pre-attentive four — needs approval. Sun marker hue: rose-amber is horizon-only, so the marker must be amber (defensible: amber = time) — flag. The arc is a permanent mark spending emptiness budget. Largest DESIGN_FACE rewrite of any concept.
**Weiser test:** Arc+marker — periphery: day-shape, daylight left ✓. Pegs — periphery: something's coming; tap: what ✓. Clock ✓. Weather sky ✓. Nothing fails, but the arc itself must earn its permanent pixels every empty day.
**Risk:** Most novel = highest learning cost and highest chance of reading as "widget"; the continuous marker drift honors the letter of the motion law but skirts its spirit; density creep is real when pegs + veil + marker are all active.

## 4. HONEST PIXEL — the Nothing face

**One-line thesis:** The provocation is typographic honesty — the display admits it's a pixel grid, speaking one rationed dot-matrix voice that gives exact answers (time, temperature) with zero learning curve.

**Info architecture:** Doto 120 clock center; temp numeral + one 18 px condition hairline (4 marks max: sun-arc / cloud-line / droplet / frost-tick) at a fixed upper-left anchor; slot band for timer/next event as in concept 2. Field does phase only. Day one fully legible. Cut: weather-from-field ambitions, everything else per the never-list.
**Weather treatment:** Explicit — a 22 px bone numeral answers "jacket?" and one hairline mark answers "bike?". Legible where v1 wasn't because it stops encoding and just says it.
**Clock treatment:** Doto 120 px center — the hero, per Nothing's rationing discipline.
**Typography:** Doto 120 + Doto 44 (timer) + Departure Mono 22 (temp, slot). The pairing is the identity: two grid-born faces, dots and pixels.
**Field/gradient behavior:** Reverts to pure time-of-day phase + air violet — its one legible job, done well.
**Motion:** State eases only; breathing kept.
**Spec deltas:** Directly bends "abstract before display" and the icon prohibition — the biggest philosophical bend, unambiguously needs Josch's sign-off; permanent corner marks spend Yohaku continuously.
**Weiser test:** Passes mechanically (each mark informs from periphery, rewards attention) but the temp numeral on a mild day informs no decision — the weakest pass of the four.
**Risk:** It is one hairline away from the Tidbyt/StandBy corner-weather cliché the precedents lens documented as the field's most-copied failure; two numeral systems are squint-confusable; the provocation lives entirely in the type voice.

---

# Recommendation

The council picks **THREE STRATA**, with Open Sky's absolute sky-band encoding adopted as its FIELD stratum and Open Sky's grain texture held as a prototype-gated add-on. It is the only concept where two lenses independently converged on the same architecture (user-context's daypart matrix and encoding's proposal B), it fixes v1's actual failure mode — relative weather encoding and a clock nobody needed at that size — with the fewest spec bends, and it is the only one that gets *calmer* (smaller clock, more emptiness, an empty slot that literally means "nothing needs you") while answering the three checks the residents currently pull phones for: bike-or-tram at 8:00, timer while cooking, when-does-tomorrow-start at 22:00. Open Sky is the purist alternative to show Josch alongside it (he may love the dare), Complication is the wildcard worth a single mockup for its unshipped day-shape idea, and Honest Pixel should be presented only as the fallback if the tercile abstraction proves too coarse in living use — it is the cliché wearing better type. The one decision only Josch can make: whether the droplet hairlines are an acceptable bend of "abstract before display," and where between 80 and 120 px his clock lands.

---
---

# Appendix: full lens reports

## LENS: calm-canon

# Calm-Tech Canon Research — Mechanisms for the Kachel Face

## 1. Concrete mechanisms in the canon

**Weiser & Brown (1996):** calm tech "moves easily from the periphery of our attention to the center, and back" — the user, not the device, chooses when to promote information. The **Dangling String** (Jeremijenko) encodes network traffic *rate* as motion *intensity*: a continuous analog mapping with no symbols, legible without looking directly. ([calmtech.com](https://calmtech.com/papers/coming-age-calm-technology))

**Amber Case:** technology should require "the smallest possible amount of attention," "communicate without speaking," and "amplify the periphery." ([calmtech.com](https://calmtech.com))

**Google Little Signals (2022)** — six distinct mechanisms ([littlesignals.withgoogle.com](https://littlesignals.withgoogle.com/), [designboom](https://www.designboom.com/design/google-little-signals-ambient-notifications-04-26-2022/)):
- **Air** — *indirect display*: puffs of air move a nearby plant; you read the signal through its effect on the environment, not the device.
- **Button** — *fill-level + user-pulled detail*: grows as information accumulates; twist right for more detail, left for less. Quantity is ambient; detail is opt-in.
- **Movement** — *positional/ordinal encoding*: seven pegs whose height/position represent calendar or timer state; fixed spatial anchors, tap for input.
- **Rhythm** — melody qualities encode urgency/tone (sound; not portable here).
- **Shadow** — *sub-perceptual presence*: a soft shadow "gently breathing when active, stretching in response to presence." Aliveness at the edge of perception.
- **Tap** — quantized urgency (impact strength = importance).

**mui Board:** the *disappearing display* — a wood plank whose dot-matrix glows only when needed, then recedes to plain material; a drawn line slowly fades as lights dim. Absence of display **is** the resting state; information "comes alive on touch." ([muilab.com](https://muilab.com/en/products_and_services/muiboard/), [Dezeen](https://www.dezeen.com/2023/01/16/mui-board-lab-smart-home-control-ces/), [Spectrum Tokyo](https://spctrm.design/interviews/the-mui-board-blending-technology-into-the-rhythm-of-everyday-life/))

**Ishii, ambientROOM / Water Lamp & Pinwheels:** map digital quantities onto *natural phenomena* processed in background awareness — bits fall as "rain" projected as water-ripple shadows; pinwheels spin in a "bit wind." ([Semantic Scholar](https://www.semanticscholar.org/paper/d8fb841c302ca341323b7020aa33f5767ad33d30), [ResearchGate](https://www.researchgate.net/publication/2447397_Water_Lamp_and_Pinwheels_Ambient_Projection_of_Digital_Information_into_Architectural_Space))

## 2. What translates to a fixed 480×480 LCD, no sound, no actuators

| Mechanism | LCD translation |
|---|---|
| Indirect display (Air, Ishii's rain) | Weather rendered as *behavior of the field* — texture, grain, veiling — not icons or subtle tint shifts |
| Fill-level + twist-for-detail (Button) | Quiet gauges that fill as events approach; tap = "twist right" for text detail |
| Positional pegs (Movement) | Fixed-anchor tick marks; position = identity, brightness/fill = proximity |
| Breathing shadow (Shadow) | Already present (horizon breathing); its *character* can carry one datum within the 6 cpm cap |
| Periphery↔center (Weiser) | Tap promotes a datum to center for seconds, then it self-decays back |
| Disappearing display (mui) | The clock itself recedes; the field is primary |

Rate-as-intensity (Dangling String) is constrained: idle loops are forbidden, so encode via *static texture* and *breathing depth*, never animation rate above the cap.

## 3. Three implementable ideas

**A. Weather as material behavior of the field (Air + Ishii).** Stop *tinting* for weather; make the field *behave*. Rain: vertical anisotropy in the dither grain (streaked texture) + cooled slate horizon; snow: brightened granular ground band; fog: lifted low-contrast veil compressing the gradient. Texture orientation and contrast are pre-attentive, static (no motion-law violation), and readable at 3 m in one glance. Precip-expected-in-12h can pre-shade the upper field — forecast as gathering weather, not a symbol.

**B. Day-arc pegs with fill (Movement + Button).** Bottom edge: up to three bone-hue ticks at fixed anchors = next calendar events; each fills/brightens as the event nears, shifting amber only inside one hour (time hue). Empty edge = free day (Yohaku). Tap a tick = event text for ~8 s, then decay (twist-for-detail; periphery→center→periphery).

**C. Recede the clock; the field is the display (mui + Weiser).** Drop 176 px to a small dot-matrix numeral (NDot-style — the Nothing nod) at a fixed anchor, ~64 px. The gradient becomes the primary datum (phase + weather). Tap anywhere: time blooms to center for 5 s in one 500 ms ease, then recedes. The face gets *calmer* (less foreground) while carrying strictly more information.

---

## LENS: user-context

## Kachel FACE — user-centered information research (workday + weekend)

**Evidence anchors.** Pizza et al.'s in-vivo smartwatch study: 50% of watch uses are pure timekeeping, and 72% of all uses are sub-5s glances — wrists and phones already own HH:MM; a wall tile's differentiated value is *context, not redundant time*. Oulasvirta et al.: home device "checking habits" are brief, context-triggered inspections of dynamic content (weather before leaving, next obligation) — exactly the checks an ambient face can pre-empt so the phone stays pocketed. Kitchen voice-assistant studies (Sciuto et al. 2018) show timers dominate kitchen device use. Conclusion: the face should answer the *check the residents were about to perform*, per daypart.

### 1. Daypart × information matrix
Dayparts keyed to sun + behavior triggers, not fixed clock. **P** = primary (≤2 s at 2–3 m), **p** = peripheral (field/hue), **–** = off.

| Datum | Rush (wake→dep.) | Day | Cooking* | Evening | 22–00 | 00–06 |
|---|---|---|---|---|---|---|
| Weather as decision (bike/transit, layers) | **P** | p | – | p (tint) | – | – |
| Clock HH:MM | p (smaller) | **P** | p | **P** | **P** (amber, small) | – (tap) |
| Next-thing slot (amber) | first event = leave-by | – | **timer = P** | tomorrow's first event | – | – |
| Air (dust-violet field shift) | p | p | p, escalation suppressed† | p | – | – |
| Fan/filter, PM2.5 numbers, Bring, agenda | – | – | – | – | – | – |

\* Cooking = event-state (timer running OR PM2.5 spike), overlays any daypart. † Frying spikes are expected; suppress violet escalation while cooking, escalate only if it persists ~30 min after.

### 2. Primary datum per daypart — against clock-always-primary
- **Rush:** The morning question is never "what time is it" (oven, phone, watch answer that); it's *"can I bike, what do I wear, when must I leave."* Weather — today a sub-legible gradient modifier — should be the rush primary: a **derived tri-state** (dry-bike / rain-transit / cold-layer) from condition + precip-12h + temp, legible in ≤2 s. First event feeds the slot as *leave-by*, not as agenda. Clock demotes to peripheral.
- **Cooking:** Timer, unconditionally. The only hard-real-time datum in the home; strongest evidence base of any kitchen display content.
- **Day (flat mostly empty on workdays):** Clock + sky as scenery. Spend nothing; weekend identical — deliberate tasks (Bring) get a deliberate swipe.
- **Evening:** The wind-down check is *"when does tomorrow start"* (alarm/bedtime decision). Tomorrow's first event in the slot is more valuable than a 176 px clock.
- **Night:** 22–00 the only question is "how late is it" — small amber clock. 00–06: emptiness; time on tap.

### 3. Never on the resting face
Raw sensor values (PM2.5, AQI, filter %) — derived states only; system/maintenance status (filter, Wi-Fi/MQTT health) — repair is deliberate-attention territory; Bring list or count — pull data, Household tile; more than one calendar event — agendas are phone work; seconds or animated colon; multi-day/numeric forecasts; precip millimetres; any notification stream; stats/streaks/persuasion; date (borderline — reward a tap, don't spend resting pixels).

### 4. Fixed anchors × time-varying relevance
Relevance moves through **intensity and fill, never position** — three permanent strata:
1. **FIELD** (whole background): atmosphere = time-phase + weather + air hue. Weather graduates from modifier to *subject* during rush — same channel, higher legibility, then relaxes.
2. **MARK** (center): clock. Position eternal; size/weight/brightness step per daypart via one 300–800 ms ease (anchor = position, not scale).
3. **SLOT** (bottom band): one amber slot meaning "the next thing that needs you" — leave-by event → timer (preempts) → tomorrow's first event → empty. Amber stays the single time-hue; an empty slot *is* the yohaku signal: nothing needs you.

This lets the face feel calmer than v1 (clock shrinks, emptiness increases) while carrying more usable information at exactly the moments the residents currently reach for phones.

Sources: [Pizza et al., Smartwatch in vivo, CHI 2016](https://dl.acm.org/doi/10.1145/2858036.2858522) · [Oulasvirta et al., Habits make smartphone use more pervasive](https://link.springer.com/article/10.1007/s00779-011-0412-2)

---

## LENS: encoding

# Ambient Data Encoding Lens — Weather/Air Legibility on the Kachel Face

## 1. Why the Orb works and v1's gradient modifiers don't

The Ambient Orb / Weather Beacon are learnable because they satisfy four conditions v1 violates:

- **Exclusive channel ownership.** The Orb's *entire hue axis* means one thing (warmer→red, cooler→blue; pulse = precipitation). On Kachel, hue is already spent: time-of-day phase owns the gradient's hue trajectory. Weather arrives as a *modifier on a moving baseline* — the observer must mentally subtract "what would 17:40 in July normally look like?" before reading the residual as weather. That's conjunction search, not pre-attentive pop-out. Pre-attentive perception detects *absolute* feature differences, not deviations from a remembered time-varying norm.
- **Discriminable levels.** Peripheral vision resolves ~4–7 hue steps and coarse lightness steps. "Cloudy flattens, rain cools+darkens" are 1-JND nudges by design (calm), so they fall below the discrimination threshold *by intent* — subtlety and legibility were traded in the same channel.
- **A stable reference frame.** The Orb has a card on the fridge: color→meaning, fixed forever. v1 has no invariant anchor; the same "flattened slate" could be dusk, clouds, or both.
- **Change salience.** The Orb changes rarely and discretely; a gradient drifts continuously, so weather transitions never produce a noticeable event to learn from.

Lesson: **one datum per channel, absolute encoding, fixed anchor, few levels.** Matthews/Forlizzi's glanceability work says the same: abstraction helps, but only when the mapping is simple, positionally stable, and low-cardinality.

## 2. Channels still free under the hue law

- **Position** — strongest free channel. A *dedicated fixed weather zone* (SPEC's spatial-anchor rule already endorses this) makes weather absolute, not relative.
- **Discrete element count** — 0/1/2/3 marks (droplets, cloud strata) is pre-attentive numerosity up to ~4.
- **Horizon line shape/behavior** — crisp vs. diffused edge (fog), lowered/veiled sun band (overcast). Shape is legal; it costs no hue.
- **Fill level** — vertical extent of a band (precip mm next 12 h as fill). Classic, learnable.
- **Texture/granularity** — dither density is *already in the pipeline*; coarse grain = precipitation is nearly free on RGB565+Bayer. Risk: reads as banding artifact at 2–3 m; needs prototyping.
- **Lightness within owned hues** — slate band lightness for cloud cover is allowed (slate = atmosphere) if steps are ≥3 clear levels, not continuous.
- Motion is effectively closed (≤6–10 cpm breathing only); saturation is reserved as alert currency.

## 3. The minimal-glyph question

**Against:** Weiser/Case purism — a glyph is "display," not periphery; Little Signals and mui deliberately avoid iconography, using material behavior instead; every permanent mark spends the Yohaku emptiness budget, and emptiness *is* the all-well signal. A sun glyph on a sunny day is redundant with the field.

**For:** Matthews et al. find abstract *and symbolic* marks outperform pure-ambient encodings on learnability; symbols self-label where fields need a legend. The Orb shipped with a reference card — a tiny glyph *is* the card, embedded. Tidbyt/TRMNL/e-ink dashboards converge on hairline glyph + numeral because temperature is *not* derivable from any field encoding — it's a quantity, and "do I need a jacket?" is the actual kitchen question. Weiser's one-line test arguably *passes*: a 20 px numeral informs from periphery (presence/absence, rough magnitude) and rewards deliberate attention (exact value). Verdict: a single, fixed-position, bone-colored, hairline mark ≤22 px is calm-legal; a row of icons is not.

## 4. Three proposals

**A. Pure field — "Weather owns the sky band."** Split gradient semantics: bottom 2 stops = time-of-day (unchanged), top 160 px = weather-owned slate band. Cloud cover → band lightness, 3 steps (clear = near-black/absent, partly = L0.25, overcast = L0.40 flat). Precip next 12 h → band descends as a soft veil toward horizon, fill-level ∝ mm, 3 steps. Fog → horizon edge diffuses 40 px. Readable after a week? *Cloud/precip yes* (absolute lightness + fill at fixed position); *rain vs. snow no*; temperature absent. Calmest; still fails "is it cold?"

**B. Hybrid (recommended). Field A + one anchored mark.** At fixed anchor (56, 388 — left of guest-card row): 1–3 droplet strokes, 2 px hairline, bone L0.80, appearing only when precip >0.5 mm/12 h; count = intensity tercile; frozen precip = same strokes rotated to ticks. Absent = dry — absence stays meaningful (Yohaku). One 500 ms ease-in on state change, then static. 2 s at 2.5 m: yes — presence/count of 3 marks at a known corner is pre-attentive; the field carries mood, the mark carries the decision ("bike or tram?").

**C. Info-forward, still calm.** Shrink clock to ~120 px (Josch: "doesn't need to be that big"), freeing the upper-left anchor: temp numeral 22 px Nothing-style dot-matrix (ndot-flavored — distinct from Inter clock, cheap as bitmap font), bone hue, plus one 18 px condition hairline mark (sun-arc/cloud-line/droplet/tick — 4 glyphs max, one meaning each). Field reverts to pure time-of-day (its legible job). Fully readable in <2 s day one, no learning week — but permanently spends emptiness; two numeral systems on one face risks clock/temp confusion at a squint. Choose C only if B's tercile abstraction proves too coarse in living use.

**Council position:** B — restore channel exclusivity (field=atmosphere, mark=precipitation decision), keep temperature for tile 3 or C-fallback.

Sources: [Ambient Devices Weather Beacon](https://ambientdevices.myshopify.com/products/weather-beacon), [Ambient device (Wikipedia)](https://en.wikipedia.org/wiki/Ambient_device), [Matthews & Forlizzi, Designing Glanceable Peripheral Displays](https://digitalassets.lib.berkeley.edu/techreports/ucb/text/EECS-2006-113.pdf), [Heuristic evaluation of ambient displays (CHI)](https://dl.acm.org/doi/10.1145/642611.642642), [Google Little Signals](https://littlesignals.withgoogle.com/), [Little Signals — designboom](https://www.designboom.com/design/google-little-signals-ambient-notifications-04-26-2022/), [InfoCanvas](https://www.researchgate.net/publication/27521310_Artistically_Conveying_Peripheral_Information_with_the_InfoCanvas)

---

## LENS: typography

# Typography research — Nothing lens for the Kachel face

## 1. Nothing's actual type system

- **NDot** (Colophon Foundry, 2021; cuts **NDot-55** and tighter **NDot-57**): the dot-matrix display face. **Licensing: proprietary and exclusive to Nothing** — the EULA restricts use to "Nothing brand materials," forbids modification and redistribution. The floating downloads (onlinewebfonts, the `xeji01/nothingfont` GitHub repo) are unlicensed copies of the real files, not clones; "Ndot-55" lookalikes (e.g. fontmirror's "LED Dot-Matrix") have murky provenance. **Verdict: no NDot cut is legally usable. Do not ship it.**
- **NType 82** (+ Mono; same Colophon/Seventy Agency program): their grotesk, inspired by IBM 1980s mainframe-era digital type — *not* Aeonik (that's CoType Foundry; the resemblance is coincidental family likeness). Equally proprietary.
- **How Nothing OS deploys them — the transferable lesson**: NDot is rationed to *hero glance moments* — lock-screen/AOD clock, widget numerals, weather-icon dot matrix; an experimental "Dot Matrix title" toggle extends it to headers. NType handles all body/settings text. **Discipline to copy: dot face = one hero datum only; everything secondary gets a quiet face.**

## 2. Is dot-matrix on a 480×480 LCD honest?

Yes — *conditionally*. A dot glyph is an idealized abstraction of the panel's own pixel grid, and the LED-alarm-clock heritage is native to kitchens (calm retro-tech, not skeuomorphism). It turns fake when dots are antialiased blobs at arbitrary sizes or animated like faux-LED flicker. **Honesty rule: every dot lands on whole pixels, rendered static.**

**Bayer-dither interaction (real risk):** 4-bpp antialiased round-dot edges over an ordered-dither gradient create two competing high-frequency patterns → shimmer/moire at glance distance. Mitigations: (a) prefer square/low-roundness dots — hard edges rasterize with near-binary alpha, nothing to beat against; (b) keep dot pitch a multiple of the Bayer cell (4 px); (c) locally deepen the gradient behind the clock (a quiet zone, which also serves Yohaku).

## 3. Candidates

| Font | License | Character | Fit |
|---|---|---|---|
| **Doto** (Google Fonts, Óliver Lalan) | **OFL-1.1** | Variable, monospace, 6×10 dot matrix; axes: wght (dot size) + ROND (0=square→100=circle); GF Latin Core → umlauts ✓; tabular by construction | **Best NDot-energy without the lawsuit.** Geometric, calm, distinctly Nothing-adjacent |
| **Departure Mono** (Helena Zhang) | **OFL** | 775 glyphs, Latin-1 ✓; pixel-perfect at **multiples of 11 px** | At 110–121 px reads chunky-terminal, harsher than dot. But **22 px = exactly 2× native grid** — crisp secondary |
| **Handjet** (Rosetta/Březina) | **OFL-1.1** | Axes wght 100–900, ELSH 0–16 (23 element shapes), ELGR | Beautiful, but element counts vary per glyph → pixel-snapping unreliable; more expressive than calm |
| **DSEG** (keshikan) | OFL-1.1 | 7/14-segment | Reads "microwave." Wrong register for a resting face; maybe timer-card only |
| Space Grotesk / Instrument Sans / Overused Grotesk | OFL | Quiet grotesks | Fallback if dot pairing feels costumey |

## 4. Recommended system

- **Hero numerals: Doto**, static instance ≈ **wght 300–350, ROND 0–50** (square-leaning dots — kills dither moire, honest to the pixel grid), at **120 px** for HH:MM (down from 176 px per brief; still <2 s readable at 3 m). 120 px → ~12 px dot cell = 3× the 4-px Bayer cell. Since LVGL fonts are pre-rasterized via `lv_font_conv`, freeze the instance, render once, and verify integer dot pitch offline — reject any size where dots straddle pixels (test 100/120; avoid odd sizes).
- **Secondary text: Departure Mono 22 px** (2× its 11-px grid → every pixel crisp at 4 bpp, umlauts included). Pairing logic: both faces are grid-born — one speaks in dots, one in pixels — a coherent "the display admits it's a display" language. If two lo-fi faces feel costumey in mockups, swap secondary to **Space Grotesk 22 px** and keep Doto as the lone provocation.
- **Flash budget**: 3 sizes total — Doto 120 (digits+colon only, subsetted), Departure Mono 22, plus one Doto ~44 px for timer countdown (keeps amber timer in the same dot voice).

Sources: [NDot — Fonts In Use](https://fontsinuse.com/typefaces/233367/ndot), [NType 82 — Fonts In Use](https://fontsinuse.com/typefaces/233368/ntype-82), [nothingfont mirror (EULA text)](https://github.com/xeji01/nothingfont), [Nothing OS 3.0 dot matrix — Android Authority](https://www.androidauthority.com/nothing-os-3-hands-on-3488739/), [Doto — GitHub](https://github.com/oliverlalan/Doto), [Doto — Google Fonts](https://fonts.google.com/specimen/Doto), [Departure Mono](https://www.departuremono.com/), [Handjet — Rosetta](https://github.com/rosettatype/handjet)

---

## LENS: precedents

# Competitive Precedent Audit — Smart-Home Resting Faces

## 1. The Cliché List (what makes Josch say "boring")

- **Giant centered HH:MM in a neutral grotesk, white-on-black.** Echo Show, Nest Hub default, StandBy default, every HA dashboard, every Divoom face. It says "I am a clock appliance," not "I am a room." Our v1 commits this exact sin at 176 px.
- **Weather icon + temp, top-right (or top-left) corner.** Universal since the first Chumby. Icon-grammar (sun/cloud glyphs) is *display* not *abstraction* — SPEC §5 already bans it, and it's the single most copied pattern in the space.
- **Grid of widget cards.** The HA wall-dashboard house style: Mushroom/button-card grids, 8–20 tiles, every datum given equal visual weight, zero hierarchy, glowing icons at 2 a.m. The anti-Yohaku. Also StandBy's widget stacks — Apple shipped the cliché with rounded corners.
- **Content churn as "liveliness."** Echo Show's rotating cards (now with injected full-screen ads) is the cautionary tale: ambient surface → billboard. Users describe eroded *trust* — the device moved without being asked. Confirms our no-idle-loop law is a differentiator, not a limitation.
- **Retro-pixel nostalgia** (Tidbyt, LaMetric, Divoom): charming, but the aesthetic *is* the product; information density is a ticker, motion is constant scroll. Nothing to steal for a calm face except Tidbyt's lesson that **a warm physical/material frame buys forgiveness** — our OKLCH warmth must do that work in pixels.
- **E-ink "productivity dashboard"** (TRMNL): lists, agendas, QR codes — desk-tool energy, not room energy.

## 2. The 3–4 Genuinely Good Moves (steal these)

1. **Nest Hub low-light clock**: when the room darkens, everything collapses to *one* dim element; brightness floor is user-tunable; Ambient EQ matches display color temperature to room light so the screen reads as an object, not a light source. Steal: **state collapse** — the face sheds layers as attention-need drops, and the darkest state is the most designed one. (Their "Timeless Dark" analog on deep red ground is the rare face with an actual palette opinion.)
2. **StandBy Night Mode**: monochrome single-hue tint (red; ours = amber, already in SPEC) applied to *everything* — not just the clock. Steal the totality: at night, no element escapes the night palette. Also: StandBy proves distance-legible "one big datum" per face works; its failure is that swiping between faces replaces hierarchy.
3. **Horology's complication discipline**: watchmakers rank legibility ruthlessly — time always wins; each complication gets a fixed sub-dial *position* (learnable by location, not label); a "big date" beats a date window because one datum earns size; overloading the dial "detracts from the dignity of the watch." This is our SPEC §5 spatial-anchor rule with 300 years of validation. Steal: **sector thinking** — zones of the 480×480 own meanings permanently, and most zones are usually empty.
4. **Analog-at-a-glance wisdom**: an analog watch is read as a *shape* (hand angle = pre-attentive), not digits. No smart display does this. A day-arc / sun-position form would give time-of-day as shape, letting the digital clock shrink.

## 3. Opportunities Nobody Takes (calm-tech theory says they'd work)

- **Emptiness as signal.** Every competitor fills the screen when idle; none dares to make "nothing shown" mean "all is well." A face that visibly *quiets* when life is fine is unoccupied territory.
- **Light-as-weather.** Everyone icons the weather; nobody makes the display *behave* like the sky — rain as darkened, slow-cooled field; fog as lifted luminance floor. Our gradient already gestures here; the fix is amplitude + one readable weather *figure* (e.g., precipitation as fill-level), not icons.
- **Day-shape awareness.** No display renders "what kind of day is left" — daylight remaining, next-event proximity — as a single spatial form (arc, horizon position). Little Signals moves air and shadow; no shipped product moved this into a screen.
- **Escalation as design material.** Competitors have two states: idle and notification-slam. A graded ladder (static → fade → pulse → decay) is literally unshipped by anyone.

**Verdict:** the field's center of gravity is *clock + icon + widget grid + churn*. Every hard constraint in SPEC §5 is an inversion of a documented industry failure — the redesign should shrink the clock, zone the dial like a watchmaker, collapse states like Nest's low-light mode, and let weather be the field itself.

Sources: [Nest Ambient EQ](https://support.google.com/googlenest/answer/9137130?hl=en), [Nest low-light community threads](https://www.googlenestcommunity.com/t5/Speakers-and-Displays/How-to-disable-low-light-mode-on-nest-hub-gen-2/m-p/613820), [Apple StandBy](https://support.apple.com/guide/iphone/use-standby-iph878d77632/ios), [StandBy red mode](https://www.tenorshare.com/ios-17/why-standby-mode-is-red.html), [Echo Show ads backlash](https://www.androidpolice.com/amazon-echo-show-disruptive-full-screen-ads/), [Tidbyt review](https://macwright.com/2022/03/11/tidbyt-review.html), [complication legibility](https://en.wikipedia.org/wiki/Complication_(horology)), [dial design](https://watchlab.sg/blog/watch-dials/)
