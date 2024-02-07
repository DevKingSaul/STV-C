#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct Vote {
    uint16_t currentPrefrence;
    uint16_t *candidates;
};

struct EliminationTable {
    uint16_t size;
    uint16_t *eliminated;
};

struct VoteList {
    uint32_t numVotes;
    Vote *votes;
};

uint32_t GetThreshold(uint16_t numWin, uint32_t numVotes) {
    if (numVotes == 0 || numWin == 0) {
        return 0;
    }

    uint32_t returnValue = numVotes / numWin;
    if (numVotes % numWin != 0) {
        return returnValue + 1;
    }

    return returnValue;
}

#define ELIMINATED_VALUE 0xffffffff
#define ELIMINATED_VALUE_16 0xffff

/*
Vote Index
Check if its already at its last (dont do anything)
Check if next candidate is eliminated
Increment Vote Table
Update Prefrence Index in the vote
*/
unsigned int next_ballot(uint32_t voteIndex, VoteList voteList, uint32_t *voteTable, uint16_t numCand) {
    if (voteIndex >= voteList.numVotes) {
        #ifdef DEBUG
        printf("next_ballot; Vote Index out of bounds\n");
        #endif
        return 1;
    }

    struct Vote vote = voteList.votes[voteIndex];

    uint16_t NextCandidate = vote.currentPrefrence + 1;

    for (;;) {
        if (NextCandidate >= numCand) {
            break;
        }

        uint16_t CandidateIndex = vote.candidates[NextCandidate];

        if (voteTable[CandidateIndex] == ELIMINATED_VALUE) {
            NextCandidate++;
            continue;
        }

        vote.currentPrefrence = NextCandidate;
        voteTable[CandidateIndex]++;
        break;
    }
    return 0;
}

/*
Stop looping:
count the number of winners elected in the round, count the number of candidates elimianted.
if both counters are 0, raise a loop execption.
*/

struct Result {
    uint8_t returnCode;
    uint16_t numWin;
    uint16_t *winners;
    uint16_t runoffIndex;
};

/* Return Codes
0 - Success
1 - Failed malloc
2 = Zero Votes
3 = Invalid Threshold (Internal Error, most likely)
4 = Candidate Index out of bounds (Critical Error)
5 = Infinite Loop
6 = Candidate Index out of bounds (Default Winner)
*/

// If every candidate has 0 votes, just eliminate all no need to go through every one since their all gonna be eliminated anyway. (Have to investigate for Default Winners)
// Update tempEliminationTable to directly pass its eliminated array, perhaps make a infinite loop instead of increasing the call stack.
int FindEliminate(uint16_t preferenceIndex, EliminationTable *eliminationTable, EliminationTable *tempEliminationTable, uint32_t **preferenceTable, uint16_t numCand) {
    if (eliminationTable->size == 1) {
        return 0;
    } else {
    Loop:
        // Check if last prefrence, change nothing.
        if (preferenceIndex >= numCand) {
            return 0;
        }

        tempEliminationTable->size = 0;

        uint32_t *subBreferenceTable = preferenceTable[preferenceIndex];

        uint32_t LowestVotes = ELIMINATED_VALUE; // Elimination Count

        #ifdef DEBUG
        printf("Preference Index: %u\n", preferenceIndex);
        #endif

        for (uint16_t i = 0; i < eliminationTable->size; i++) {
            uint16_t CandidateIndex = eliminationTable->eliminated[i];
            uint32_t CandidateVotes = subBreferenceTable[CandidateIndex];

            if (LowestVotes == ELIMINATED_VALUE || LowestVotes >= CandidateVotes) {
                if (LowestVotes > CandidateVotes) {
                    tempEliminationTable->size = 0;
                }

                LowestVotes = CandidateVotes;
                tempEliminationTable->eliminated[tempEliminationTable->size] = CandidateIndex;
                tempEliminationTable->size++;
            }
        }

        // Copy results over to Elimination Table
        eliminationTable->size = tempEliminationTable->size;
        memcpy(eliminationTable->eliminated, tempEliminationTable->eliminated, sizeof(uint16_t) * tempEliminationTable->size);

        #ifdef DEBUG
        printf("Elimination (findElim):");
        for (int i = 0; i < eliminationTable->size; i++) {
            printf("\n  %u", eliminationTable->eliminated[i]);
        }
        printf("\n");
        #endif

        if (eliminationTable->size == 1) {
            return 0;
        } else if (eliminationTable->size > 1) {
            // look into if this is correct
            preferenceIndex++;
            goto Loop;
            //return FindEliminate(preferenceIndex + 1, eliminationTable, tempEliminationTable, preferenceTable, numCand);
        } else {
            return 1; // Error
        }
    }
}

/*
Candidate Index
Vote List
Vote Table

Set Candidate Table to Elimianted
Traverse Vote Table to find elimianted and do next_ballot
*/
int Eliminate_Single(uint16_t EliminatedCandidate, uint32_t *voteTable, VoteList voteList, uint16_t numCand) {
    #ifdef DEBUG
    printf("Eliminate_Single: %u\n", EliminatedCandidate);
    #endif
    voteTable[EliminatedCandidate] = ELIMINATED_VALUE;

    for (uint32_t VoteIndex = 0; VoteIndex < voteList.numVotes; VoteIndex++) {
        struct Vote vote = voteList.votes[VoteIndex];
        uint16_t CandidateIndex = vote.candidates[vote.currentPrefrence]; // Get Current Prefrence for the Vote
        if (CandidateIndex != EliminatedCandidate) continue;
        next_ballot(VoteIndex, voteList, voteTable, numCand);
    }

    return 0;
}

Result count_stv(VoteList voteList, uint16_t numCand, uint16_t numWin) {
    Result result;
    result.returnCode = 0;
    result.numWin = 0;
    result.winners = (uint16_t *)malloc(sizeof(uint16_t) * numWin);
    result.runoffIndex = ELIMINATED_VALUE_16;

    if (result.winners == NULL) {
        result.returnCode = 1;
        return result;
    }

    if (voteList.numVotes == 0) {
        result.returnCode = 2;
        return result;
    }

    uint32_t threshold = GetThreshold(numWin, voteList.numVotes);
    uint16_t winnersLeft = numWin;
    uint16_t candidatesLeft = numCand;

    if (threshold == 0 || threshold > voteList.numVotes) {
        result.returnCode = 3;
        return result;
    }

    uint32_t *voteTable = (uint32_t *)malloc(sizeof(uint32_t) * numCand); // voteTable[Candidate]

    if (voteTable == NULL) {
        result.returnCode = 1;
        return result;
    }

    uint32_t **preferenceTable = (uint32_t **)malloc(sizeof(uint32_t *) * numCand); // preferenceTable[Preference][Candidate], used for calculating run-offs. Uses numCand^2 size

    if (preferenceTable == NULL) {
        free(voteTable);
        result.returnCode = 1;
        return result;
    }

    // Allocate memory for preferenceTable
    for (uint16_t i = 0; i < numCand; i++) {
        preferenceTable[i] = (uint32_t *)malloc(sizeof(uint32_t) * numCand);

        if (preferenceTable[i] == NULL) {
            for (int i2 = 0; i2 < i; i++) {
                free(preferenceTable[i2]);
            }
            free(voteTable);
            free(preferenceTable);
            result.returnCode = 1;
            return result;
        }

        // Initalize Sub-Preference Table
        for (uint16_t i2 = 0; i2 < numCand; i2++) {
            preferenceTable[i][i2] = 0;
        }
    }

    for (uint16_t i = 0; i < numCand; i++) {
        voteTable[i] = 0;
    }

    // Count votes
    for (uint32_t VoteIndex = 0; VoteIndex < voteList.numVotes; VoteIndex++) {
        struct Vote vote = voteList.votes[VoteIndex];
        uint16_t CandidateIndex = vote.candidates[0]; // Get First Preference (0) for the Vote
        if (CandidateIndex >= numCand) {
            #ifdef DEBUG
            printf("CRITICAL ERROR: Candidate Index out of bounds\n");
            #endif
            result.returnCode = 4;
            goto end_NoElim;
        }

        voteTable[CandidateIndex]++;

        // Count votes for each preference
        for (uint16_t Preference = 0; Preference < numCand; Preference++) {
            uint16_t CandidateIndex = vote.candidates[Preference]; // Get Preference for the Vote
            if (CandidateIndex >= numCand) {
                #ifdef DEBUG
                printf("CRITICAL ERROR: Candidate Index out of bounds\n");
                result.returnCode = 4;
                #endif
                goto end_NoElim;
            }

            preferenceTable[Preference][CandidateIndex]++;
        }
    }

    struct EliminationTable eliminationTable;
    eliminationTable.eliminated = (uint16_t *)malloc(sizeof(uint16_t) * numCand);

    if (eliminationTable.eliminated == NULL) {
        result.returnCode = 1;
        goto end_NoElim;
    }

    // Used tempoarily by FindEliminate
    struct EliminationTable tempEliminationTable;
    tempEliminationTable.eliminated = (uint16_t *)malloc(sizeof(uint16_t) * numCand);

    if (tempEliminationTable.eliminated == NULL) {
        result.returnCode = 1;
        goto end;
    }

    uint32_t LowestVotes; // Elimination Count

    while(winnersLeft > 0 && candidatesLeft > 0) {
        #ifdef DEBUG
        printf("\n\nNEW ROUND\n");
        #endif
        eliminationTable.size = 0;
        LowestVotes = ELIMINATED_VALUE; // Reset

        unsigned char HadSurplus = 0;
        uint16_t RoundWinners = 0; // For checking for Infinite Loops

        for (uint16_t CandidateIndex = 0; CandidateIndex < numCand; CandidateIndex++) {
            uint32_t CandidateVotes = voteTable[CandidateIndex];
            if (CandidateVotes == ELIMINATED_VALUE) continue;
            if (CandidateVotes >= threshold) {
                voteTable[CandidateIndex] = ELIMINATED_VALUE;
                result.winners[result.numWin] = CandidateIndex;
                result.numWin++;
                winnersLeft--;
                candidatesLeft--;

                RoundWinners++;

                #ifdef DEBUG
                printf("Candidate %u Won!\n", CandidateIndex);
                #endif

                if (CandidateVotes > threshold) {
                    int VoteCounter = 0;
                    for (uint32_t VoteIndex = 0; VoteIndex < voteList.numVotes; VoteIndex++) {
                        struct Vote vote = voteList.votes[VoteIndex];
                        uint16_t VoteCandidateIndex = vote.candidates[vote.currentPrefrence]; // Get Current Prefrence for the Vote

                        if (VoteCandidateIndex != CandidateIndex) continue;
                        VoteCounter++;
                        
                        if (VoteCounter > threshold) {
                            #ifdef DEBUG
                            printf("SURPLUS: %u, %u\n", VoteCounter - threshold);
                            #endif
                            next_ballot(VoteIndex, voteList, voteTable, numCand);
                        }
                    }

                    HadSurplus = 1; // Do new round, dont check for eliminations
                    break; // Break so it does it immedietaly to prevent miscalculation for other candidates who may have been affected by the surplus.
                }
            } else {
                if (LowestVotes == ELIMINATED_VALUE || LowestVotes >= CandidateVotes) {
                    if (LowestVotes > CandidateVotes) {
                        eliminationTable.size = 0;
                    }

                    LowestVotes = CandidateVotes;
                    eliminationTable.eliminated[eliminationTable.size] = CandidateIndex;
                    eliminationTable.size++;
                }
            }
        }

        if (HadSurplus == 1) {
            continue;
        }

        // Check for "Default" Winneres
        if (candidatesLeft <= winnersLeft) {            
            uint16_t CandidateCounter = 0; // Internal Check, to prevent Buffer Overflow
            for (uint16_t CandidateIndex = 0; CandidateIndex < numCand; CandidateIndex++) {
                if (voteTable[CandidateIndex] == ELIMINATED_VALUE) continue;
                CandidateCounter++;
                if (CandidateCounter > candidatesLeft) {
                    printf("ERROR! WTF!!\n");
                    result.returnCode = 6;
                    goto end;
                }

                result.winners[result.numWin] = CandidateIndex;
                result.numWin++;
                #ifdef DEBUG
                printf("Candidate %u Won by Default!\n", CandidateIndex);
                #endif
            }
            break;
        }

        #ifdef DEBUG
        printf("Elimination");
        for (int i = 0; i < eliminationTable.size; i++) {
            printf("\n%u", eliminationTable.eliminated[i]);
        }
        printf("\n");
        #endif

        FindEliminate(1, &eliminationTable, &tempEliminationTable, preferenceTable, numCand);

        #ifdef DEBUG
        printf("Post Find-Elimination");
        for (int i = 0; i < eliminationTable.size; i++) {
            printf("\n%u", eliminationTable.eliminated[i]);
        }
        printf("\n");
        #endif

        if (eliminationTable.size == 1) {
            Eliminate_Single(eliminationTable.eliminated[0], voteTable, voteList, numCand);
            candidatesLeft--;
        } else if (eliminationTable.size > 1) {
            printf("Multi-elim not implement. %u %u\n", candidatesLeft, eliminationTable.size);

            /* Check if this would cause a Win Default where it doesnt suffice the amount of Winners Left. Example: 
                Candidates Left = 4
                Winners Left = 2
                Eliminated = 3

                There would be 1 candidate left after this elimiantion, not furfilling the 2 needed. (Candidates Left - Eliminated) < Winners Left.
                If so just default all of them, set the runoff index.

            Example where there is just enough left:
                Candidates Left = 4
                Winners Left = 2
                Eliminated = 2

                Here there would be 2 candidates left, leaving enough to furfill all positions.
            */ 

           if ((candidatesLeft - eliminationTable.size) < winnersLeft) {
            result.runoffIndex = result.numWin;
            uint16_t *newPtr = (uint16_t *)realloc(result.winners, sizeof(uint16_t) * (result.numWin + candidatesLeft));//malloc(sizeof(uint16_t) * numWin);
            
            if (newPtr == NULL) {
                result.returnCode = 1;
                goto end;
            }

            result.winners = newPtr;

            uint16_t CandidateCounter = 0; // Internal Check, to prevent Buffer Overflow
            for (uint16_t CandidateIndex = 0; CandidateIndex < numCand; CandidateIndex++) {
                if (voteTable[CandidateIndex] == ELIMINATED_VALUE) continue;
                CandidateCounter++;
                if (CandidateCounter > candidatesLeft) {
                    printf("ERROR! WTF!!\n");
                    result.returnCode = 6;
                    goto end;
                }

                result.winners[result.numWin] = CandidateIndex;
                result.numWin++;
                #ifdef DEBUG
                printf("Candidate %u Runoff Elimination!\n", CandidateIndex);
                #endif
            }
            break;
           }

            for (uint16_t i = 0; i < eliminationTable.size; i++) {
                Eliminate_Single(eliminationTable.eliminated[i], voteTable, voteList, numCand);
                candidatesLeft--;
            }
            //break;
        } else if (RoundWinners == 0) {
            result.returnCode = 5;
            break;
        }
    }

end:
    if (eliminationTable.eliminated != NULL) {
        free(eliminationTable.eliminated);
    }

    if (tempEliminationTable.eliminated != NULL) {
        free(tempEliminationTable.eliminated);
    }

end_NoElim:
    if (voteTable != NULL) {
        free(voteTable);
    }

    if (preferenceTable != NULL) {
        for (int i = 0; i < numCand; i++) {
            if (preferenceTable[i] == NULL) continue;
            free(preferenceTable[i]);
        }

        free(preferenceTable);
    }

    return result;
}
