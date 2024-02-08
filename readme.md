This is a custom version of Single Transferable Vote which uses a custom threshold and redistrbution method.

Candidates are not passed, but rather votes has an array, going from High to Low prefrence votes. Each Prefrence species the "index" of the candidates. And the program returns a list of Candidate Indexes. This lets us take advantage of the nature of how arrays work instead of having to work with Dynamically Allocated Strings. You also have to specify the number of candidates.

For example, say you have Candidate 1, Candidate 2 and Candidate 3. You set numCand to 3. Lets say a vote is Candidate 2, Candidate 1 and Candidate 3 (in that order) the vote represented in a Unsigned 16-bit array would be [1, 0, 2]

The way you would implement this is to map Candidates to Candidate Numbers (starting from 0), and translating the votes to match those Candidate Numbers. Then when you get the results you get the Candidate from the Candidate Numbers returned.

## THIS CODE IS INCOMPLETE AND STILL IN DEVELOPMENT.
