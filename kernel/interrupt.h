#pragma once

void interrupt_enable();
bool interrupt_disable();
void interrupt_restore(bool was_enabled);

void interrupt_enable_timer();
void interrupt_enable_external();
