#pragma once

enum charger_state {
	CHARGER_NOT_CHARGING = 0,
	CHARGER_CHARGING,
	CHARGER_COMPLETE,
};

int charger_init(void);
enum charger_state charger_read_state(void);
