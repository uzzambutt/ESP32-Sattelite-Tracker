# Workspace Rules

- **Hamlib / rotctld TCP Integration:** When modifying TCP command parsers for satellite tracking (e.g., Look4Sat, GPredict), adhere strictly to the rotctld protocol. 
  - For `p` (get position), return ONLY the Azimuth and Elevation separated by a newline (e.g., `123.4\n56.7\n`).
  - For `P` (set position), return ONLY `RPRT 0\n` on success.
  - NEVER append generic debug messages or `"OK\n"` to network streams handling rotctld, as this will crash the client parser.

- **Design & Documentation Aesthetic:** Maintain a premium, high-end "SpaceX / Tesla" aesthetic for all Web UI and documentation. Do NOT use any emojis in code comments, commit messages, or documentation files.
