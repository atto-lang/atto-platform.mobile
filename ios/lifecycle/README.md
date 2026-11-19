# iOS lifecycle

Treat scene generation as revocable. Foreground -> background revokes UI submission until foreground admission is refreshed. Teardown permanently retires scene identity, cancels queued intents, and fences callbacks; a stale callback may not recreate a scene. UI work must run on main actor. iOS may suspend or terminate background applications; no perpetual actor-service promise.
