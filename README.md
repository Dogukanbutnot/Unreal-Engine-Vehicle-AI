# Unreal-Engine-Vehicle-Traffic-AI
 Advanced City AI & Traffic System Architecture
1. Vehicle Intelligence (NPC Driving Logic)
Obstacle Detection & Avoidance: Implementation of Line Trace and Sphere Trace systems to detect static or dynamic obstacles (e.g., player cars, abandoned vehicles).

Audio Feedback System: Integration of a DoOnce node logic for realistic horn usage upon obstacle detection.

Overtaking & Lane Changing: Vector-based math to calculate safe overtaking paths using Right Vector offsets and VInterp To for smooth steering.

Adaptive Safety Scanning: Rear and side-view "radar" (Multi-Line Trace) to check blind spots before initiating maneuvers.

2. Traffic Control & Intersection Management
Traffic Light Integration: Event-driven communication between traffic light actors and NPC vehicles using Trigger Boxes.

Maneuver Locking: A logic gate system that disables lane changing near intersections (bIsNearIntersection) to prevent chaotic collisions.

Chain Reaction Braking: Implementation of distance-based speed adjustments to create a natural "queue" effect at red lights.

3. Reactive World & AI Temperament
Panic State (Threat Response): High-priority state override (bIsPanicking) triggered by damage or weapon fire.

Rule Violation Logic: Dynamic bypassing of traffic laws (ignoring red lights, exceeding speed limits) when the AI is under threat.

Escape Vector Calculation: Mathematical calculation of "Away From Threat" directions using player-to-actor vector inversion.

4. Pedestrian & Crowd Systems (Mass Entity)
Sidewalk Navigation (Spline Pathing): Using ZoneGraph and Splines to guide pedestrians along sidewalks with organic offsets to prevent "conga-line" movements.

Crosswalk Synchronization: Smart waiting slots and "Green Light" listeners for pedestrians to cross safely.

Emergency Braking (Pedestrian Safety): Dynamic collision volumes that scale with vehicle speed to ensure instant stopping if a pedestrian enters the road.

5. Optimization & Scalability
ECS Architecture (Mass Entity): Planning for high-performance crowd simulation by separating data (Fragments) from logic (Processors).

LOD (Level of Detail) Management: Strategy for switching between high-fidelity AI and lightweight background data based on camera distance.
