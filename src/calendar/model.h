namespace cal {

class GUITask;

class Model {
  public:
	Model() = default;

	void registerGUITask(GUITask* task);

	void reserveEvent(int minMinutes);

	void reserveEventUntilNext();

	void freeCurrentEvent();

  private:
	GUITask* _guiTask;
};
}  // namespace cal