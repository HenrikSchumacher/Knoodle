public:

/*!@brief Return whether this diagram is _locked_. Public UNSAFE operations cannot be called while this is being locked to prohibit unintended modifications that break topological invariance.*/
bool LockedQ() const
{
    return lockedQ;
}

/*!@brief Unlock diagram complex to allow topological modifications.*/
void Unlock()
{
    lockedQ = false;
}

/*!@brief Lock diagram complex to prohibit topological modifications.*/
void Lock()
{
    lockedQ = true;
}

private:

void LockMessage( const std::string & tag ) const
{
    Msgr::wprint(tag, "This method is considered **UNSAFE**, and the diagram is currently locked to prevent break of topological invariance. If you want to perform this operation anyways, call `Unlock()` first. (Don't forget to `Lock()` it again.)");
}

