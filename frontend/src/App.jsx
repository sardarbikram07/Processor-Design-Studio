import { useState } from 'react';
import ConfigBuilder from './pages/ConfigBuilder';
import PipelineVisualizer from './pages/PipelineVisualizer';
import Dashboard from './pages/Dashboard';
import { IconConfig, IconPipeline, IconDashboard, IconCpu } from './components/Icons';
import './index.css';

const TABS = [
  { id: 'config',    label: 'Config Builder',    Icon: IconConfig,    component: ConfigBuilder },
  { id: 'pipeline',  label: 'Pipeline Visualizer', Icon: IconPipeline,  component: PipelineVisualizer },
  { id: 'dashboard', label: 'Comparison Dashboard', Icon: IconDashboard, component: Dashboard },
];

export const API = 'http://localhost:3001';

export default function App() {
  const [activeTab, setActiveTab] = useState('config');
  const [lastResult, setLastResult] = useState(null);

  const ActivePage = TABS.find(t => t.id === activeTab)?.component;

  return (
    <div className="app">
      <nav className="nav">
        <a className="nav-brand" href="#">
          <img src="/logo.png" alt="Processor Studio Logo" className="nav-logo-img" />
          <span>Processor Design Studio</span>
        </a>
        <div className="nav-tabs">
          {TABS.map(t => {
            const Icon = t.Icon;
            return (
              <button
                key={t.id}
                className={`nav-tab ${activeTab === t.id ? 'active' : ''}`}
                onClick={() => setActiveTab(t.id)}
              >
                <Icon size={16} color={activeTab === t.id ? 'var(--neon-cyan)' : 'var(--text-sec)'} />
                <span>{t.label}</span>
              </button>
            );
          })}
        </div>
      </nav>
      <div className="main">
        {ActivePage && (
          <ActivePage
            lastResult={lastResult}
            onResult={setLastResult}
            onGoTo={setActiveTab}
          />
        )}
      </div>
    </div>
  );
}
