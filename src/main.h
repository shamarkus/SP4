
#ifndef MAIN_H
#define MAIN_H

void addIncidentToEmail(struct DatabaseRecord* dr, struct Threshold* th, struct EmailInfoList* el, struct TimeElement* start, const struct IncidentType* incidentType, struct CCPair* ccPairLLHead, bool headerExists);

#endif
